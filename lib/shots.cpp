#if _PROFILE
# pragma optimize( "", off )
#endif

// shots and explosions simulation

#include "wpch.hpp"
#include "shots.hpp"
#include "global.hpp"
#include <El/Common/randomGen.hpp>
#include <El/Common/perfProf.hpp>
#include "world.hpp"
#include "lights.hpp"
#include "landscape.hpp"
#include "camera.hpp"
#include "vehicleAI.hpp"
#include "AI/ai.hpp"
#include "weapons.hpp"
#include "Network/network.hpp"
#include "txtPreload.hpp"
#include "Shape/specLods.hpp"
#include "diagModes.hpp"
#include "Protect/selectProtection.h"
#include <El/Enum/enumNames.hpp>
#include <El/QStream/qbStream.hpp>
#include "gameStateExt.hpp"
#include "paramArchiveExt.hpp"
#include "UI/missionDirs.hpp"
#include "drawText.hpp"

#if _VBS2 // suppression
#include "soldierOld.hpp"
#endif

#if _DEBUG
#define ARROWS 1
#endif


Object::ShotInfo::ShotInfo(const Shot *shot, EntityAI *owner, Object *hit, Vector3Par modelPos)
:owner(owner),modelPos(modelPos)
{
  if (shot)
  {
    local = shot->IsLocal();
    ammo = shot->Type();
    ignoreSmall = true;
    originDir = shot->OriginDirection();
  }
  else
  {
    local = !owner ||owner->IsLocal();
    ammo = nullptr;
    ignoreSmall = false;
    originDir = hit->FutureVisualState().DirectionModelToWorld(modelPos);
  }
}

DEFINE_CASTING(Shot)

Shot::Shot(EntityAI *parent, const AmmoType *type)
:base(type->GetShape(), type, CreateObjectId()), _parent(parent)
{
  if (type->GetShape() == NULL)
    RptF("No shape for ammo type %s",(const char *)type->GetName());
  _timeToLive = type->_timeToLive;
  SetSimulationPrecision(type->simulationStep);
  SetTimeOffset(0);
}

Object::Visible Shot::VisibleStyle() const
{
#if _VBS2 //no speed limitation, since tracers are also rendered as objects
  return _shape.IsNull() ? ObjInvisible : ObjVisibleFast;
#else
  if (CHECK_DIAG(DECombat))
    return ObjVisibleFast;
  return (FutureVisualState()._speed.Distance2(GLOB_SCENE->GetCamera()->Speed()) > Square(400.0f)) || _shape.IsNull() ? ObjInvisible : ObjVisibleFast;
#endif
}

bool Shot::CanPenetrateFreely(float penetrability) const
{
  // assume that objects with penetrability smaller than 1.000 (i.e. right now only leaves) are freely penetrable
  // note that caliber is taken into account too and if no penetrability is given (i.e. penetrability is zero), the object is treated as non-penetrable
  return (penetrability > 0.0f) && (penetrability <= 1000.0f * Type()->caliber);
}

void Shot::Sound(bool inside, float deltaT)
{
  PROFILE_SCOPE_EX(snSht,sound);
  const VisualState &vs = RenderVisualState();
  const SoundPars &sound=Type()->_soundFly;
  if (!_sound && sound.name.GetLength() > 0 && GSoundScene->CanBeAudible(WaveEffect,vs.Position(),sound.vol))
    _sound = GSoundScene->OpenAndPlay(sound.name,NULL,false,vs.Position(),vs.Speed(),sound.vol,sound.freq,sound.distance);
  if (_sound)
  {
    float obstruction = 1.0f;
    float occlusion = 1.0f;
    GWorld->CheckSoundObstruction(NULL,false,obstruction,occlusion);
    _sound->SetObstruction(obstruction,occlusion,deltaT);
    _sound->SetVolume(sound.vol,sound.freq);
    _sound->SetPosition(vs.Position(),vs.Speed());
  }
}

void Shot::UnloadSound()
{
  _sound.Free();
}

#if _VBS3

#include <El/Evaluator/express.hpp>

void Shot::OnShotEvent
(
  AmmoEvent event,
  EntityAI *shoter,
  Entity   *hit,
  Vector3   pos,
  Vector3   velocity,
  AutoArray<RString> &component,
  bool explode,
  Vector3   normal
)
{
  GameValue gpos   = GWorld->GetGameState()->CreateGameValue(GameArray);
  GameValue gvel    = GWorld->GetGameState()->CreateGameValue(GameArray);
  GameValue gcomStr = GWorld->GetGameState()->CreateGameValue(GameArray);
  GameValue gAmmoStat = GWorld->GetGameState()->CreateGameValue(GameArray);
  GameValue gnormal = GWorld->GetGameState()->CreateGameValue(GameArray);

  GameArrayType &posArgu = gpos;
  posArgu.Add(GameValue(pos.X()));
  posArgu.Add(GameValue(pos.Z()));
  posArgu.Add(GameValue(pos.Y()));

  GameArrayType &velArgu = gvel;
  velArgu.Add(GameValue(velocity.X()));
  velArgu.Add(GameValue(velocity.Z()));
  velArgu.Add(GameValue(velocity.Y()));

  GameArrayType &normArgu = gnormal;
  normArgu.Add(GameValue(normal.X()));
  normArgu.Add(GameValue(normal.Z()));
  normArgu.Add(GameValue(normal.Y()));

  GameArrayType &comArgu = gcomStr;
  for( int i = 0; i < component.Size(); ++i)
    comArgu.Add(GameValue(component[i]));

  GameArrayType &ammoStat = gAmmoStat;
  ammoStat.Add(GameValue(Type()->hit)); 
  ammoStat.Add(GameValue(Type()->indirectHit)); 
  ammoStat.Add(GameValue(Type()->indirectHitRange)); 
  ammoStat.Add(GameValue(Type()->explosive)); 
  ammoStat.Add(GameValue(Type()->GetName())); 

  GameValue value = GWorld->GetGameState()->CreateGameValue(GameArray);
  GameArrayType &arguments = value;
  arguments.Add(GameValueExt(this));
  arguments.Add(GameValueExt(shoter));
  arguments.Add(GameValueExt(hit));
  arguments.Add(gpos);
  arguments.Add(gvel);
  arguments.Add(gcomStr);
  arguments.Add(gAmmoStat);
  arguments.Add(gnormal);
  arguments.Add(GameValue(explode));

  Type()->OnEvent(event,value);
}
#endif

void ShotShell::AmmoChangeExpectedFailed()
{
  // detect if the cheaters is only testing the hack
  // TODO: detect memory breakpoints somewhere in the higher level
  // memory breakpoint usually means hack development

  // when single player only, we do not care
  // in developer version we want to be able to test the countermeasures in SP, though
#if !_ENABLE_CHEATS
  if (GWorld->GetMode()==GModeNetware)
#endif
  {
    // when there is only one identity, we do not care
    // little harm can be done this way
    // this is most likely the cheater testing his cheat
#if !_ENABLE_CHEATS
    const AutoArray<PlayerIdentity> *identities = GetNetworkManager().GetIdentities();
    if (identities && identities->Size()>1)
#endif
    {
      // TODO: we also do not care much if player is hitting his own "local" entities

      // cheat detected - what will we do?
      // sometimes we turn the bullet into a fake one - it will cause no damage to anyone
      // sometimes we decide to harm the player
      float value = GRandGen.RandomValue();
      if (value > 0.5f)
      {
        if (value > 0.9f)
        {
          // sometimes we harm the player
          if (_parent)
          {
            DoDamageResult result;
            _parent->LocalDamage(result,ShotInfo(this,_parent,_parent,VZero),Type()->hit,_parent->GetRadius()*0.5f);
            _parent->ApplyDoDamage(result,_parent, RString(),SkinnedPosition());
          }
        }
        else
          // sometimes all we do is making a ghost bullet out of this one
          _real = false;
      }
    }
  }
}

LSError Shot::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.SerializeRef("Parent", _parent, 1))
  CHECK(ar.Serialize("timeToLive", _timeToLive, 1, 10.0))
  return LSOK;
  
/* TODO: ?? serialize
  InitPtr<const AmmoType> _type;
  Ref<AbstractWave> _sound;
  bool _fake;
*/
}

#define SHOT_MSG_LIST(XX) \
  XX(Create, CreateShot) \
  XX(UpdateGeneric, None) \
  XX(UpdateDamage, None) \
  XX(UpdatePosition, None)

DEFINE_NETWORK_OBJECT(Shot, base, SHOT_MSG_LIST)

DEFINE_NET_INDICES_EX(CreateShot, NetworkObject, CREATE_SHOT_MSG)

DEFINE_NET_INDICES_EX_ERR(UpdateShot, UpdateVehicle, UPDATE_SHOT_MSG, NoErrorInitialFunc)

NetworkMessageFormat &Shot::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  Vector3 temp = VZero;
  Matrix3 tempM = M3Identity;
  switch (cls)
  {
  case NMCCreate:
    NetworkObject::CreateFormat(cls, format);
    CREATE_SHOT_MSG(CreateShot, MSG_FORMAT)
    break;
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_SHOT_MSG(UpdateShot, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

Shot *Shot::CreateObject(NetworkMessageContext &ctx)
{
  PREPARE_TRANSFER(CreateShot)

  OLinkO(EntityAI) parent;
  if (TRANSF_REF_BASE(parent, parent) != TMOK)
    return NULL;

  RString typeName;
  if (TRANSF_BASE(type, typeName) != TMOK)
    return NULL;
  Ref<EntityType> type = VehicleTypes.New(typeName);
  AmmoType *aType = dynamic_cast<AmmoType *>(type.GetRef());
  if (!aType)
    return NULL;

  Ref<Shot> shot = NewShot(parent, aType, NULL, NULL);
  if (shot->TransferMsg(ctx) != TMOK)
    return NULL;

  NetworkId objectId;
  if (TRANSF_BASE(objectCreator, objectId.creator) != TMOK) return NULL;
  if (TRANSF_BASE(objectId, objectId.id) != TMOK) return NULL;
  shot->SetNetworkId(objectId);
  shot->SetLocal(false);

  GWorld->AddFastVehicle(shot);
  GWorld->AddSupersonicSource(shot);
  GWorld->AddFakeBulletsSource(shot);
  return shot;

/*
  base *veh = base::CreateObject(ctx);
  Shot *shot = dyn_cast<Shot>(veh);
  if (!shot) return NULL;

  if (shot->TransferMsg(ctx) != TMOK) return NULL;
  return shot;
*/
}

TMError Shot::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(NetworkObject::TransferMsg(ctx))
    }
    {
      PREPARE_TRANSFER(CreateShot)

      if (ctx.IsSending())
      {
        TRANSF_REF(parent)
        RString type = GetName() ? GetName() : "";
        TRANSF_EX(type, type)
        Vector3 pos = FutureVisualState().Position();
        TRANSF_EX(createPos, pos)
        Matrix3 orient = FutureVisualState().Orientation();
        TRANSF_EX(createOrient, orient)
      }
      else
      {
        Vector3 pos;
        Matrix3 orient;
        pos.Init();
        TRANSF_EX(createPos, pos)
        TRANSF_EX(createOrient, orient)
        SetPosition(pos);
        SetOrientation(orient);
      }
      TRANSF_EX(createSpeed, FutureVisualState()._speed)
      TRANSF(timeToLive)
    }
    break;
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    break;
  default:
    TMCHECK(base::TransferMsg(ctx))
    break;
  }
  return TMOK;
}

float Shot::CalculateError(NetworkMessageContextWithError &ctx)
{
  return base::CalculateError(ctx);
}

float Shot::GetMaxPredictionTime() const
{
  // shots behave very predictably (mostly ballistic trajectory)
  // and therefore they can be predicted reliably for very long time
  // there are also no updates abouts shots, so prediction is necessary
  return 100.0f;
}

static const EnumName EngineStateNames[]=
{
  EnumName(Missile::Init, "INIT"),
  EnumName(Missile::Thrust, "THRUST"),
  EnumName(Missile::Fly, "FLY"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(Missile::EngineState dummy)
{
  return EngineStateNames;
}

static const EnumName LockStateNames[]=
{
  EnumName(Missile::Locked, "LOCKED"),
  EnumName(Missile::Lost, "LOST"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(Missile::LockState dummy)
{
  return LockStateNames;
}

LSError Missile::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.SerializeRef("Target", _target, 1))
  CHECK(ar.Serialize("thrust", _thrust, 1))
  CHECK(ar.SerializeEnum("engine", _engine, 1))
  CHECK(ar.SerializeEnum("lock", _lock, 1))
  CHECK(ar.Serialize("effects", _effects, 1))
  return LSOK;

/* TODO: ?? serialize
  Ref<AbstractWave> _soundEngine;

  CloudletSource _cloudlets;

  float _initTime,_thrustTime;

  Color _lightColor;
  Ref<LightPointOnVehicle> _light;
*/
}

#define MISSILE_MSG_LIST(XX) \
  XX(Create, CreateShot) \
  XX(UpdatePosition, UpdatePositionVehicle)

DEFINE_NETWORK_OBJECT(Missile, base, MISSILE_MSG_LIST)

float Missile::CalculateError(NetworkMessageContextWithError &ctx)
{
  // if missile is guided, we should send some updates about it
  // base::CalculateError was zero for Shot in past, but now more robust solution
  // using GetNMType was implemented
  return base::CalculateError(ctx);
}

float Missile::GetMaxPredictionTime() const
{
  // if we did not receive any updates, continue predicting
  return 100.0f;
}


DEFINE_CASTING(PipeBomb)

PipeBomb::PipeBomb(EntityAI *parent, const AmmoType *type)
:Shot(parent,type)
{
  _explosion = false;
  _timeToLive = FLT_MAX;
}

void PipeBomb::Simulate(float deltaT, SimulationImportance prec)
{
  PROFILE_SCOPE_EX(simPB,sim);
  _timeToLive-=deltaT;
  if (IsLocal() && (_explosion || _timeToLive < 0)
#if _EXT_CTRL
    && !IsExternalControlled()
#endif
    )
  {
    // WIP:EXTEVARS:FIXME fill hitInfo properly (normal, dir)
    HitInfo hitInfo(Type(), FutureVisualState().Position(), VZero, VUp);
    GLandscape->ExplosionDamage(_parent,VZero,NULL,VZero,hitInfo);

    // Check if there exists a event for ammoHit?
#if _VBS3
    if(Type()->IsEventHandler(AEAmmoHit))
    {
      AutoArray<RString> noComponents;
      OnShotEvent(AEAmmoHit,_parent,NULL,FutureVisualState().Position(),_speed,noComponents);
    }
    if(Type()->IsEventHandler(AEAmmoExplode))
    {
      AutoArray<RString> noComponents;
      OnShotEvent(AEAmmoExplode,_parent,NULL,FutureVisualState().Position(),_speed,noComponents);
    }
#endif
    SetDeleteRaw();
  }
}

static const float TimeStartTimer = 30.0f;
static const float TimeSetTimer = 30.0f;

bool PipeBomb::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
  switch (action->GetType())
  {
  case ATDeactivate:
    // No parameters
    return true;
  case ATStartTimer:
    params.Add(CreateTextASCII(NULL, Format("%.0f", TimeStartTimer)));
    return true;
  case ATSetTimer:
    params.Add(CreateTextASCII(NULL, Format("%.0f", TimeSetTimer)));
    params.Add(CreateTextASCII(NULL, Format("%.0f", GetTimer())));
    return true;
  }
  return base::GetActionParams(params, action, unit);
}

void PipeBomb::PerformAction(const Action *action, AIBrain *unit)
{
  if (!unit) return;
  Person *person = unit->GetPerson();
  if (!person) return;

  switch (action->GetType())
  {
    case ATStartTimer:
      {
        SetTimer(TimeStartTimer);
      }
      break;
    case ATSetTimer:
      {
        float time = GetTimer();
        if (time > 1e6) time = 0;
        SetTimer(time + TimeSetTimer);
      }
      break;
    case ATDeactivate:
      {
        SetDelete();
        person->RemovePipeBomb(this);
          
        RString name;
        const AmmoType *ammo = Type();
        if (ammo) name = ammo->_defaultMagazine;
        if (name.IsEmpty()) name = "PipeBomb";
        person->CreateItem(name);
      }
      break;
    default:
      base::PerformAction(action,unit);
      break;
  }
}

void PipeBomb::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  base::GetActions(add, unit);

  if (!unit) return;
  Person *person = unit->GetPerson();
  if (!person) return;

  bool free = unit->IsFreeSoldier();
  if (free)
  {
    bool owner = person->HasPipeBomb(this);
    if (owner)
    {
      UIActionType type = GetTimer() > 1e6 ? ATStartTimer : ATSetTimer;
      UIAction action(new ActionBasic(type, this));
      add(action);
    }
    if (owner || person->CanDeactivatePipeBombs())
    {
      UIAction action(new ActionBasic(ATDeactivate, this));
      add(action);
    }
  }
}

LSError PipeBomb::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("explosion", _explosion, 1, false))
  return LSOK;
}


DEFINE_CASTING(Mine)

Mine::Mine(EntityAI *parent, const AmmoType *type)
: Shot(parent,type)
{
  _timeToLive = GRandGen.PlusMinus(0.5, 0.1);
  _active = true;
}

/// helper for Mine::Simulate
class CheckProximityMine
{
  Vector3 _pos;
public:
  explicit CheckProximityMine(Vector3Par pos):_pos(pos) {}
  bool operator()(Entity *veh) const
  { 
    return (veh->GetMass() >= 10000.0f) && (_pos.Distance2(veh->FutureVisualState().Position()) <= Square(6.0f));
  }
};


void Mine::Simulate(float deltaT, SimulationImportance prec)
{
  PROFILE_SCOPE_EX(simMn,sim);
  if (!_active)
  {
    // change orientation to indicate mine is no longer active
    // do not touch Direction
    // change DirectionAside
    float bank = FutureVisualState().DirectionAside().Y();
    float bankWanted = 0.5f;
    if (bank >= bankWanted)
      return;
    float delta = bankWanted-bank;
    saturate(delta, -deltaT * 0.5f, +deltaT * 0.5f);
    Matrix3 rotZ(MRotationZ, delta);
    Matrix3 newOrient = FutureVisualState().Orientation() * rotZ;
    SetOrientation(newOrient);
    return;
  }

  _timeToLive -= deltaT;
  if (IsLocal() && (_timeToLive < 0.0f)
#if _EXT_CTRL
    && !IsExternalControlled()
#endif
    )
  {
    SECUROM_MARKER_SECURITY_ON(12)

    bool found = GWorld->ForEachVehicle(CheckProximityMine(FutureVisualState().Position()));
    if (found)
    {
#if _VBS3
      // we're about to go boom...
      if(Type()->IsEventHandler(AEAmmoHit))
      {
        AutoArray<RString> empty;
        OnShotEvent(AEAmmoHit,_parent,NULL,Position(),_speed,empty);
      }      
      if(Type()->IsEventHandler(AEAmmoExplode))
      {
        AutoArray<RString> empty;
        OnShotEvent(AEAmmoExplode,_parent,NULL,Position(),_speed,empty);
      }      
#endif
      // WIP:EXTEVARS:FIXME fill hitInfo properly (normal, dir)
      HitInfo hitInfo(Type(), FutureVisualState().Position(), VZero, VUp);
      GLandscape->ExplosionDamage(_parent, VZero, NULL, VZero, hitInfo);
      SetDeleteRaw();
    }
    else
      _timeToLive = GRandGen.PlusMinus(0.5, 0.1);
    SECUROM_MARKER_SECURITY_OFF(12)
  }
}

bool Mine::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
  switch (action->GetType())
  {
  case ATDeactivateMine:
  case ATTakeMine:
    // No parameters
    return true;
  }
  return base::GetActionParams(params, action, unit);
}

void Mine::PerformAction(const Action *action, AIBrain *unit)
{
  if (!unit) return;
  Person *person = unit->GetPerson();
  if (!person) return;

  switch (action->GetType())
  {
    case ATDeactivateMine:
      if (IsActive())
      {
        // only owner can process actions
        DoAssert(GetNetworkManager().IsServerOwner());
        SetActive(false);
      }
      break;
    case ATTakeMine:
      {
        DoAssert(GetNetworkManager().IsServerOwner());
        const AmmoType *ammo = Type();
        if (ammo)
        {
          RString name = ammo->_defaultMagazine;
          if (!name.IsEmpty())
          {
            Ref<Magazine> magazine = dyn_cast<Magazine>(GWorld->NewVehicleWithID(name));
            if (magazine)
            {
              if (person->CanAddItem(magazine))
              {
                // convert mine to magazine
                SetDelete();
    
                // add to the world
                GWorld->AddOutVehicle(magazine);
                magazine->SetMoveOutFlag();
      
                person->AddItem(magazine);

                // create network object
                if (GWorld->GetMode() == GModeNetware)
                  GetNetworkManager().CreateVehicle(magazine, VLTOut, "", -1);
              }
            }
          }
        }
      }
      break;
    default:
      base::PerformAction(action,unit);
      break;
  }
}

void Mine::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  base::GetActions(add, unit);

  if (!unit) return;
  Person *person = unit->GetPerson();
  if (!person) return;

  bool free = unit->IsFreeSoldier();
  if (free && person->CanDeactivateMines())
  {
    if (IsActive())
    {
      UIAction action(new ActionBasic(ATDeactivateMine, this));
      add(action);
    }

    const AmmoType *ammo = Type();
    if (ammo)
    {
      RString name = ammo->_defaultMagazine;
      if (!name.IsEmpty())
      {
        // temporary magazine to pass into CheckMagazine
        Ref<Magazine> magazine = dyn_cast<Magazine>(GWorld->NewVehicleWithID(name));
        if (magazine)
        {
          if (person->CanAddItem(magazine))
          {
            UIAction action(new ActionBasic(ATTakeMine, this));
            add(action);
          }
        }
      }
    }
  }
}

LSError Mine::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("active", _active, 1, true))
  return LSOK;
}

#define MINE_MSG_LIST(XX) \
  XX(Create, CreateShot) \
  XX(UpdateGeneric, UpdateMine) \
  XX(UpdateDamage, None) \
  XX(UpdatePosition, None)

DEFINE_NETWORK_OBJECT(Mine, base, MINE_MSG_LIST)

#define UPDATE_MINE_MSG(MessageName, XX) \
  XX(MessageName, bool, active, NDTBool, bool, NCTNone, DEFVALUE(bool, true), DOC_MSG("Mine is active (can explode)"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE)

DECLARE_NET_INDICES_EX_ERR(UpdateMine, UpdateShot, UPDATE_MINE_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateMine, UpdateShot, UPDATE_MINE_MSG, NoErrorInitialFunc)

NetworkMessageFormat &Mine::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_MINE_MSG(UpdateMine, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError Mine::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateMine)
      TRANSF(active)
    }
    break;
  default:
    TMCHECK(base::TransferMsg(ctx))
    break;
  }
  return TMOK;
}

float Mine::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateMine)

      ICALCERR_NEQ(bool, active, ERR_COEF_MODE)
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}


DEFINE_FAST_ALLOCATOR(ShotShell)
DEFINE_CASTING(ShotShell)

ShotShell::ShotShell(EntityAI *parent, const AmmoType *type, bool real)
: base(parent, type)
{
  _real = real;
  _initDelay = type->initTime;
  _timeToExplosion = type->explosionTime;
  _fuseDistanceLeft = type->fuseDistance;
  _airFriction = type->_airFriction;
  _terminate = false;
}

void ShotShell::StartFrame()
{
  if (_terminate)
    SetDeleteRaw();
}

// only real shots should cause a damage
bool ShotShell::CanDamage() const
{
#if _EXT_CTRL
  return _real && IsLocal() && !IsExternalControlled();
#else
  return _real && IsLocal();
#endif
}

Object::Visible ShotShell::VisibleStyle() const
{
  if (_initDelay > 0.0f) return ObjInvisible;
  return base::VisibleStyle();
}

bool ShotShell::CanExplodeImmediately() const
{
  // not timed or time already elapsed
  bool canExplode = (_timeToExplosion >= FLT_MAX) || (_timeToExplosion <= 0.0f);
#if _VBS3
  return canExplode;
#else
  return canExplode && (_fuseDistanceLeft <= 0.0f);
#endif
}

void ShotShell::DebugShotShell(const CollisionInfo *info, float deltaT, Vector3Par oldSpeed, Vector3Par newSpeed, const char *action, ...)
{
#if SHOT_DEBUGGING
  const int bufLen = 64;
  static char buffer[bufLen];
  static char tempBuffer[bufLen];
  va_list args;
  va_start(args, action);
  vsnprintf(tempBuffer, bufLen, action, args);
  va_end(args);
  RStringVal shotId = GetObjectId().GetDebugName();
  RString objId = "";
  if (info)
  {
    const Object *object = info->object;
    const LODShape *shape = object->GetShape();
    const char* shapeName = shape ? GetFilenameExt(shape->Name()) : "NO_SHAPE";
    objId = Format("%s %s/%d", (const char*)object->GetObjectId().GetDebugName(), shapeName, info->component);
  }
  snprintf(buffer, bufLen, "SS %s %s %s", (const char*)shotId, tempBuffer, (const char*)objId);
  Vector3Val pos = Position();
  float oSpeed = oldSpeed.Size();
  float nSpeed = newSpeed.Size();
  Vector3Val oDir = oldSpeed.Normalized();
  Vector3Val nDir = newSpeed.Normalized();
  LogF("%-56s %.2f %.4f:  %7.3f -> %7.3f  [%8.2f,%9.4f,%8.2f] (%5.2f,%5.2f,%5.2f) -> (%5.2f,%5.2f,%5.2f)", buffer, -_initDelay + 1e-6, deltaT, oSpeed, nSpeed, pos.X(), pos.Y(), pos.Z(), oDir.X(), oDir.Y(), oDir.Z(), nDir.X(), nDir.Y(), nDir.Z());
  // to be sure that the preset prefix is used only once
#endif
}

void ShotShell::UpdatePosAndSpeed(float deltaT, Vector3Par newPos, float penetrability)
{
  // update position and fuse length
  _fuseDistanceLeft -= newPos.Distance(FutureVisualState().Position());
  if (_fuseDistanceLeft < 0.0f)
    _fuseDistanceLeft = 0.0f;
  Move(newPos);
  if (CanPenetrateFreely(penetrability))
  { // shot is in the air or in very penetrable material (like leaves) - apply air friction and gravity
    Vector3Val accel = FutureVisualState()._speed * (FutureVisualState()._speed.Size() * _airFriction);
  Vector3 grav = VUp * -G_CONST * Type()->_coefGravity;
    FutureVisualState()._speed += (accel + grav) * deltaT;
  }
}

void ShotShell::UpdatePosAndSpeed(const char *debugString, float deltaT, Vector3Par newPos, float penetrability)
{
#if SHOT_DEBUGGING
  Vector3 oldSpeed = FutureVisualState()._speed;
  UpdatePosAndSpeed(deltaT, newPos, penetrability);
  DebugShotShell(NULL, deltaT, oldSpeed, FutureVisualState()._speed, "UpdatePosAndSpeed() %s", debugString);
#else
  UpdatePosAndSpeed(deltaT, newPos, penetrability);
#endif
}

void ShotShell::FixPosAndSpeedAfterCollision(Vector3Par surfNormal)
{
// FIXME RHAL: which approach to use?
#if 0
  // reduce speed substantially, but the shot should still move (otherwise no further trajectory simulation and collision detection will be done)
  Vector3Par out = _speed.Normalized() * 0.001f;
#else
  // 1mm in the direction of the surface normal
  Vector3Par out = surfNormal * 0.001f;
#endif
  // make sure to move out a bit to prevent shots going inside objects
  Move(FutureVisualState().Position() + out);
  ///DebugShotShell(NULL, 0.0f, _speed, out, "FixPosAndSpeedAfterCollision()");
  // and move slowly in the direction of the normal too
  FutureVisualState()._speed = out;
}

void ShotShell::Sound(bool inside, float deltaT)
{
  if (_initDelay <= 0.0f)
    base::Sound(inside, deltaT);
}

static Vector3P ModelPosFromDirectHit(Object *directHit, Vector3Par pos)
{
  if (!directHit) return VZero;
  return directHit->RenderVisualState().GetInvTransform().FastTransform(pos);
}


bool ShotShell::SimulateHit(const CollisionInfo *info, Vector3Par pos, Vector3Par surfNormal, Vector3Par newSpeed)
{
  const AmmoType *ammoType = Type();
  DirectHitType *directHit = info ? info->HitObject() : NULL;
  Object *componentOwner = info ? info->object : NULL;
  int componentIndex = info ? info->component : -1;
  HitInfo hitInfo(ammoType, pos, surfNormal, FutureVisualState()._speed, newSpeed);
  float speed2 = FutureVisualState()._speed.SquareSize();
  float newSpeed2 = newSpeed.SquareSize();
  Assert (newSpeed2 <= speed2);
  // does the shot ends here ((i.e. a final impact is simulated)?
  bool fullStop = newSpeed2 < 1e-6f;
  float explosive = ammoType->explosive;

  if (fullStop || (explosive < 1.0f))
  { // final impact or (at least partially) kinetic
    if (CanDamage())
    {
      // assume damage dependent on inertia (v); for dependency on energy (v^2), this would be: kineticFactor = (speed2 - newSpeed2) / ammoType->_typicalSpeed2;
      // note that even incoming speed could be zero, i.e. the kineticFactor could be zero too
      float kineticFactor = (sqrt(speed2) - sqrt(newSpeed2)) * InvSqrt(ammoType->_typicalSpeed2);
      Assert(kineticFactor >= 0.0f);
      // assume even the fastest bullet can do no more than 2x damage
      saturateMin(kineticFactor, 2.0f);
      float energyFactor = (1.0f - explosive) * kineticFactor;
      if (fullStop)
        // shells explode only on impact (and not on penetration/deflection)
        energyFactor += explosive;
      ///DebugShotShell(info, energyFactor, _speed, newSpeed, "k");

#if SHOT_DEBUGGING
      if (energyFactor <= 0.0f)
        ErrF("Assert in SimulateHit(): energyFactor=%f, explosive=%f, kineticFactor=%f, fullStop=%d, speed=%f, newSpeed=%f, typicalSpeed=%f",
             energyFactor, explosive, kineticFactor, (int)fullStop, sqrt(speed2), sqrt(newSpeed2), sqrt(ammoType->_typicalSpeed2));
#endif
      Assert(energyFactor > 0.0f);
      float explosionFactor = (fullStop) ? 0.1f : 0.3f;
      const float hitDepth = info ? info->DistInside() : 0.f;
      GLandscape->ExplosionDamage(_parent, OriginDirection(), directHit, ModelPosFromDirectHit(directHit, hitInfo._pos), hitInfo, componentOwner, componentIndex, energyFactor, explosionFactor, hitDepth);
    }
  }
  else
  { // explosive only
    // WIP:SHOOTING: why not check on CanDamage() here?
    float kineticFactor = speed2 - newSpeed2;
    float landAlpha = floatMin(0.01f, kineticFactor);
    float scale = landAlpha * 0.2f * (GRandGen.RandomValue() * 0.3f + 0.8f);
    saturateMin(scale, 0.05f);
    float alpha = 10.0f * landAlpha * (GRandGen.RandomValue()* 0.3f + 0.7f);
    saturateMin(alpha, 1.0f);
    float timeToLive = 60.0f * scale * 5.0f;
    saturate(timeToLive, 10.0f, 30.0f);

    LODShape *shape = GLOB_SCENE->Preloaded(CraterShell);
    // crater on ground
    float azimut = GRandGen.RandomValue() * 2.0f * H_PI;

    Matrix4 transform(MIdentity);
    transform.SetOrientation(Matrix3(MRotationY,azimut));
#if !_VBS3_CRATERS_DEFORM_TERRAIN
    transform.SetScale(scale * 0.5f);
#endif
    transform.SetPosition(pos);

    // create vehicle
    RString effectsType;
    float effectsTimeToLive;
    float size = 0.5f * scale;

    Texture *tex = NULL;
    // if terrain is below water, use water style effects
    // we should move a bit up first as roadways are placed above (fire) geometry
    bool water = GLandscape->RoadSurfaceY(pos + VUp * 0.5f, NULL, NULL, &tex) < (GLandscape->WaterSurfaceY(pos) - 0.05f);

    if (water)
    {
      effectsType = "ImpactEffectsWater";
      effectsTimeToLive = 0.3f;
    }
    else
    {
      effectsType = "ImpactEffectsSmall";
      if (tex)
      {
        RString impactType = tex->GetSurfaceInfo()._impact;
        effectsType = ammoType->GetHitEffects(impactType);
      }
      effectsTimeToLive = floatMin(10.0f * size, 0.1f * timeToLive);
      saturate(effectsTimeToLive, 0.02f, 0.2f);
    }
    ///DebugShotShell(info, size, _speed, newSpeed, "e");
    // prepare evars from hitInfo
    EffectVariables evars(hitInfo);
    Crater *crater = new Crater(transform, shape, VehicleTypes.New("#crater"), effectsType, evars, effectsTimeToLive, timeToLive, size);
    crater->SetID(GLandscape->NewObjectID());
#if !_VBS3_CRATERS_DEFORM_TERRAIN
    crater->SetAlpha(alpha);
#endif
    GLOB_WORLD->AddAnimal(crater);
    // this is probably only a small hit - no need to update cache
    //GLandscape->ForceClutterUpdate(crater);
  }

  const bool hasDecal = hitInfo._ammoType->_decalShape.NotNull();
  const bool clientOnly = (!GetNetworkManager().IsServer() || !GetNetworkManager().IsClient());
  if (_real && fullStop && hasDecal && clientOnly)
  {
    const float hitDepth = info ? info->DistInside() : 0.f;

    const Person *hitPerson = dyn_cast<const Person, Object>(directHit);
    bool isMobile = (hitPerson != nullptr) && (hitPerson->Object::GetType() == TypeVehicle);
    if (isMobile && directHit)
    {
      SkinnedPosition skinning;
      if (componentOwner==directHit && componentIndex>=0)
      {
        skinning.obj = directHit;
        LODShape *lShape = directHit->GetShape();
        isMobile = (lShape->GetSkeleton() != nullptr);
        if (isMobile)
        {
          const ConvexComponents &fireComponent = lShape->GetFireComponents();
          // caution: componentIndex is transferred over network, if client/server data are not matching, we may get it wrong
          if (componentIndex>=0 && componentIndex<fireComponent.Size())
          {
            const ConvexComponent *component = fireComponent.Get(componentIndex);
            component->GetSkinning(skinning,lShape->FireGeometryLevel(),lShape);

            auto deanimated = directHit->DeanimatePoint(directHit->RenderVisualState(),lShape->FindFireGeometryLevel(),skinning,ModelPosFromDirectHit(directHit, hitInfo._pos));
            if (!deanimated.second)
            {
              skinning.bones.Clear();
            }
            else
            {
              skinning.originalPosition = deanimated.first;
            }

            isMobile = deanimated.second;
            if (isMobile && hitInfo._inSpeed.SquareSize() > 0)
            {
              // There must not be more than one bone
              // get dominant bone
              SkeletonIndex domBone = SkeletonIndexNone;
              float domW = 0;
              for (int i=0; i<skinning.bones.Size(); i++)
              {
                float w = skinning.bones[i].weight;
                if (w>domW)
                {
                  domW = w;
                  domBone = skinning.bones[i].bone;
                }
              }

              const ObjectVisualState &vs = directHit->RenderVisualState();
              Vector3 hitDirLcl(vs.GetInvTransform().Rotate(hitInfo._inSpeed.Normalized()));  //to model space
              Vector3 hitDirUpLcl(vs.GetInvTransform().Rotate(VUp));  //to model space
              if (SkeletonIndexValid(domBone))
              {
                // de-animate direction of hit
                // Get the transformation matrix from original to skinned space
                Matrix4 boneTrans = MIdentity;
                directHit->AnimateBoneMatrix(boneTrans, vs, lShape->FindFireGeometryLevel(), domBone);
                if (boneTrans.Direction().SquareSize() > 0)
                {
                  // Convert the direction from skinned to un-skinned space
                  hitDirLcl = boneTrans.InverseGeneral().Rotate(hitDirLcl);
                  hitDirUpLcl = boneTrans.InverseGeneral().Rotate(hitDirUpLcl);
                }
              }

              const float distInside = floatMinMax(1.5f * hitDepth, ProjectileDecalOnVehicle::MIN_DEPTH, ProjectileDecalOnVehicle::MAX_DEPTH);
              if (GetNetworkManager().IsServerOwner())
              {
                GLandscape->ApplyProjectileDecal(componentOwner, componentIndex, hitInfo, distInside, skinning.originalPosition, hitDirLcl, hitDirUpLcl);
              }
              else
              {
                //send to server info about the projectile decal
                GetNetworkManager().ApplyProjectileDecal(componentOwner, componentIndex, hitInfo, distInside, skinning.originalPosition, hitDirLcl, hitDirUpLcl);
              }

              //Vector3 hitPosInWorld = directHit->RenderVisualState().PositionModelToWorld(directHitModelPos);
              //enf::Shape::CreateSphere(hitPosInWorld, 0.015, 0x00ff0000, enf::SS_VISIBLE);
              //enf::Shape::CreateLine(hitPosInWorld, hitPosInWorld + hitInfo._inSpeed, 0x00aabb55, enf::SS_VISIBLE);
            }
          }
        }
      }
    }

    if (!isMobile)
    {
      if (GetNetworkManager().IsServerOwner())
      {
        GLandscape->ApplyProjectileDecal(hitInfo, hitDepth);
      }
      else
      {
        // pass the creation of the projectile loot to the server
        GetNetworkManager().ApplyProjectileDecal(nullptr, -1, hitInfo, hitDepth, hitInfo._pos, hitInfo._inSpeed, VUp);
      }
    }
  }

  return true;
}

/**
@param info collision info on the object object which is influenced by the deflection (or NULL for landscape collision)
@param normal surface normal in the point of deflection
@param isect point of the deflection
@param dist distance from current position to the impact point
@param deltaT time which still needs to be simulated
*/
bool ShotShell::SimulateDeflection(const CollisionInfo *info, Vector3Par normal, Vector3Par isect, float dist, float deltaT, float maxDeflectionAngleCos)
{
  const AmmoType *ammoType = Type();
  float maxDeflect = ammoType->deflecting;
  if ((maxDeflectionAngleCos <= 0.0f) && (maxDeflect <= 0.0f))
    return false;

  Vector3 halfway = normal;
  const float changeNormal = 0.5f;
  halfway[0] += GRandGen.Gauss(-changeNormal, 0.0f, changeNormal);
  halfway[1] += GRandGen.Gauss(-changeNormal, 0.0f, changeNormal);
  halfway[2] += GRandGen.Gauss(-changeNormal, 0.0f, changeNormal);
  halfway.Normalize();
  float cosAngle = -halfway * FutureVisualState()._speed.Normalized();

  // calculate the part of the speed going inside
  float goingInside = floatMax(-normal * FutureVisualState()._speed, 0.0f);

  // if the ammo doesn't explode just on impact, the angle is very low and the movement is very little,
  // a stable state is assumed and friction is simulated instead of deflection (FIXME: check the angle instead of the depth?)
  // note: it could be better to use roadways now to make sure grenades work with them
  if (!CanExplodeImmediately() && (goingInside < 2.0f) && (dist < 0.1f))
  {
    if (dist > 0.01f)
      // make sure the partial segment is rendered; after deflection we want no more suppression tracing, the trajectory is unpredictable
      TerminateSegment(true);
    // simulate landscape friction and forces
    // forces are based on normal, velocity is aligned to surface, gravity is applied to speed as a part of the default simulation
    // we need to apply friction here
    float friction = -100.0f * FutureVisualState()._speed.CosAngle(normal);
#if _VBS3
    friction = floatMax(friction, 1.0f);
#else
    friction = floatMin(1.0f, floatMax(friction, 0.0f));
#endif
    Vector3Par oldSpeed = FutureVisualState()._speed;
    // make sure _speed is no longer going inside (FIXME: why this?)
    FutureVisualState()._speed += goingInside * normal;
    Friction(FutureVisualState()._speed, friction * (3.0f * sign(FutureVisualState()._speed) + 0.1f * FutureVisualState()._speed), VZero, deltaT);
    DebugShotShell(info, 0.0f, oldSpeed, FutureVisualState()._speed, "d");
    SimulateMovement(deltaT, true, true);
    return true;
  }

  if ((cosAngle < 0.0f) || (cosAngle >= maxDeflect) || (cosAngle >= maxDeflectionAngleCos) || (FutureVisualState()._speed.SquareSize() <= Square(5.0f)))
    // no deflection detected
    return false;

  // reflect around normal as a halfway vector
  Vector3 newSpeed = FutureVisualState()._speed - 2.0f * halfway * (FutureVisualState()._speed * halfway);

  // if the reflected speed goes still inside, it is an impact
  if (newSpeed * normal <= 0.0f)
    return false;

  // slow down
  float slowDown = floatMax(1.0f - Square(cosAngle / maxDeflect), 0.0f);
#if _VBS2 //intensify possible slowdown
  slowDown = floatMin(0.7f, slowDown) * GRandGen.Gauss(0.1f, 0.6f, 0.8f);
#else
  slowDown = floatMin(ammoType->_deflectionSlowDown, slowDown) * GRandGen.Gauss(0.6f, 0.9f, 1.0f);;
#endif
  newSpeed *= slowDown;
  Assert((slowDown >= 0.0f) && (slowDown <= 1.0f));
  DebugShotShell(info, 0.0f, FutureVisualState()._speed, newSpeed, "D");
  SimulateHit(info, isect, normal, newSpeed);
  FutureVisualState()._speed = newSpeed;
  if (dist > 0.01f)
    // make sure the partial segment is rendered; after deflection we want no more suppression tracing, the trajectory is unpredictable
    TerminateSegment(true);
  SimulateMovement(deltaT, true);
  return true;
}

/**
@param info collision info on the object object which is influenced by the deflection (or NULL for landscape collision)
@param normal incident surface normal (normalized)
@param isect intersection point (place where shot entered the object)
@param dist distance traveled inside of the object
@param deltaT time which still needs to be simulated
returns true when the penetration occurred or false if there was no penetration and the impact should be simulated
*/
bool ShotShell::SimulatePenetration(const CollisionInfo *info, Vector3Par normal, Vector3Par isect, float dist, float deltaT)
{
  const AmmoType *ammoType = Type();
  float penetrability = info->BulletPenetrability();
  if ((penetrability <= 0.0f) || (ammoType->explosive >= 0.7f && info->BulletPenetrability() > 100))
    return false;
  // check distance traveled in the object, assume bullet deceleration is linear
  float distInside = info->DistInside();
  Assert(distInside > 0.0f);

  if(info->Thickness()>0)
  { //use thickness only when bullet enters object
    if(info->entry) 
    {//bigger the entry angle is, more material we go trough 
      float cosAngle = info->dirOutNotNorm.Normalized().DotProduct(FutureVisualState()._speed.Normalized());
      float thick = (cosAngle != 0)? (fabsf(info->Thickness() / cosAngle)) : distInside;
      //collision geometry is not thick enough
      distInside = (info->exit)? distInside = floatMin(thick,distInside) : thick;
    }
    //thickness has already been used
    else penetrability = 0.01f;
  }

  float speed = FutureVisualState()._speed.Size();
  float deceleration = distInside * penetrability / ammoType->caliber;
  if (speed <= deceleration)
    return false;
  Vector3 lDirNorm = FutureVisualState()._speed.Normalized();
  // move the bullet to the point where it goes out of the object
  Vector3 newPos = isect + distInside * lDirNorm;
  // reduce speed, change direction, continue traveling
  float reduceSpeed = deceleration / speed;
  const float changeSize = 0.25f;
  Vector3 changeDir = Vector3(GRandGen.Gauss(-changeSize, 0.0f, changeSize) * reduceSpeed,
                              GRandGen.Gauss(-changeSize, 0.0f, changeSize) * reduceSpeed,
                              GRandGen.Gauss(-changeSize, 0.0f, changeSize) * reduceSpeed);
  Vector3 dir = lDirNorm + changeDir;
  dir.Normalize();
  Vector3 newSpeed = dir * (speed - deceleration);
  // FIXME RHAL: use _speed or newSpeed here?
  float timeInside = distInside * FutureVisualState()._speed.InvSize();
  // if we did not simulate any time, we have a problem - what can we do? Next iteration will detect collision again
  Assert(timeInside > 0.0f);
  // FIXME RHAL: resolve this case properly
  Assert(timeInside <= deltaT*1.05f);
  if (timeInside > deltaT)
    timeInside = deltaT;
  // handle the in-side object part
  UpdatePosAndSpeed("penetrating", timeInside, newPos, penetrability);
  // revert time back to the moment we went out
  deltaT -= timeInside;
  DebugShotShell(info, timeInside, FutureVisualState()._speed, newSpeed, "P");
  SimulateHit(info, isect, normal, newSpeed);
  FutureVisualState()._speed = newSpeed;
  if (dist > 0.01f)
    // make sure the partial segment is rendered; after penetration we want no more suppression tracing, the trajectory is unpredictable
    TerminateSegment(true);
  SimulateMovement(deltaT, true);

  return true;
}

static SkinnedPosition CollisionSkinning(const CollisionInfo &info)
{
  LODShape *lShape = info.object->GetShape();
  const ConvexComponents &fireComponent = lShape->GetFireComponents();
  const Ref<ConvexComponent> component = fireComponent.Get(info.component);

  SkinnedPosition pos(info.object);
  component->GetSkinning(pos,lShape->FireGeometryLevel(),lShape);
  pos.originalPosition = ModelPosFromDirectHit(info.object,info.pos);
  return pos;
}


/**
@param ignoreNearCollision when deflected or during penetration, we need to ignore early collision, as we start in collision state
@param ignoreDeflection when simulating a stable state, avoid recursion
*/
void ShotShell::SimulateMovement(float deltaT, bool ignoreNearCollision, bool ignoreDeflection)
{
#if _VBS2 // suppression
  CheckForSuppression();
#endif

  if (deltaT <= 0.0f)
    return;

  Vector3 origPos = FutureVisualState().Position();
  Vector3 lDir = FutureVisualState()._speed * deltaT;
  Vector3 newPos = origPos + lDir;
  float maxDist = FutureVisualState()._speed.Size() * deltaT;

  if (maxDist <= 0.0f)
  { // even at very slow movement, gravitation must still be handled
    UpdatePosAndSpeed("no speed", deltaT, newPos);
    return;
  }

  const AmmoType *ammoType = Type();
  Vector3 collisionBeg = origPos;
  const float nearCollisionDistance = 0.1f;
  if (ignoreNearCollision)
    collisionBeg += FutureVisualState()._speed.Normalized() * nearCollisionDistance;
  float objDist = newPos.Distance(collisionBeg);
  
  // calculate intersection with land between current and new position
  Vector3 groundPos;
  bool collidingWithWater;
  float groundDist = GLandscape->IntersectWithGroundOrSea(&groundPos, collidingWithWater, origPos, FutureVisualState()._speed.Normalized(), 0.0f, maxDist);
  bool groundFarEnough = !ignoreNearCollision || (groundDist >= nearCollisionDistance);

#if _VBS3 
  // When shooting from the vehicle with personal items enabled
  // it would detect a collision with the vehicle and kill 
  // the man. Have to directly tell it here, to ignore the vehicle
  // like gunners

  bool hitObject = false;

  Man *man = dyn_cast<Man>(_parent.GetLink());

  if(man&&man->Brain()&&man->Brain()->GetVehicleIn())
    GLandscape->ObjectCollision(collision,this,man->Brain()->GetVehicleIn(),collisionBeg,position,0);
  else
  /* removed since big guns attached to vehicles would hit themselves
  if(_parent && _parent.GetLink()->IsAttached())
    GLandscape->ObjectCollision(collision,this,_parent.GetLink()->GetAttachedTo(),collisionBeg,position,0);
  else
  */
#endif //_VBS3

  // test collisions with objects
  CollisionBuffer collision;
  GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),collision, Landscape::FilterIgnoreTwo(this, _parent), collisionBeg, newPos, 0.0f);
  int numCollisions = collision.Size();
  if (numCollisions > 0)
  {
    // distance and index and distance of the nearest collision
    float minDist = FLT_MAX;
    int minI = -1;

    // shot bullets can go through glass
    // FIXME: why only bullets, what about other shots (like grenades, sabots, etc.)?
    bool detectGlass = dyn_cast<ShotBullet>(this) != NULL;
    const Texture *glass = GPreloadedTextures.New(TextureBlack);

    // find nearest colliding object
    for (int i = 0; i < numCollisions; i++)
    {
      const CollisionInfo &info = collision[i];
      if (!info.object)
        continue;
      float dist = info.under * objDist;
      // check if collision with the ground is earlier
      if (groundFarEnough && (dist >= groundDist))
        continue;
      if (detectGlass && (info.texture == glass))
        continue;
      if(info.Thickness()>0 && !info.entry)
        continue;
      if (dist < minDist)
      {
        minDist = dist;
        minI = i;
      }
    }

    if (CanDamage() && CanExplodeImmediately())
    { // damage what was hit; note that the accumulated damage is applied when the dmgAcc is destroyed at the end of this scope
      // TODO: damage the parent object when the proxy was hit
      RString ammo = ammoType->ParClass().GetName();
      DoDamageAccumulator dmgAcc;
      for (int i = 0; i < numCollisions; i++)
      {
        CollisionInfo &info = collision[i];
        if (!info.object)
          continue;
        float dist = info.under * objDist;
        if (groundFarEnough && (dist >= groundDist))
          continue;
        // stop on first solid obstacle
        if (detectGlass && (info.texture != glass))
          continue;
        if (dist > minDist)
          continue;
        // damage corresponding component
        DoDamageResult result;
        Object* dest = info.object;
        dest->DirectLocalHit(result, info.component, ammoType->hit);

        SkinnedPosition pos = CollisionSkinning(info);

        dmgAcc.Add(dest, _parent, ammo, pos, result);
      }
    }

    if (minI >= 0)
    { // handle collision with the object
      // WIP:SHOOTING: use CanDamage() here (i.e. check for _real too)?
#if _EXT_CTRL
      if (IsLocal() && !IsExternalControlled())
#else
      if (IsLocal())
#endif
      {
        const CollisionInfo *info = &collision[minI];
        float dist = minDist;
        // FIXME RHAL: remove this ignoreNearCollision hack
        if (ignoreNearCollision)
          dist += nearCollisionDistance;
        Vector3Val normal = info->dirOutNotNorm.Normalized();

        // update shot position and speed at the exact time of the collision
        Vector3Val collPos = info->pos;
        float timeDone = dist * FutureVisualState()._speed.InvSize();
        // FIXME RHAL: when ignoreNearCollision is in effect, timeDone could be to high
        //Assert(timeDone <= deltaT);
        if (timeDone > deltaT)
          timeDone = deltaT;
        UpdatePosAndSpeed("collision", timeDone, collPos);
        deltaT -= timeDone;


        Object *obj = info->object;
        EntityAI *ent = dyn_cast<EntityAI>(obj); //who got hit?
        if (ent && _real) ent->OnHitPart(*info,Type(),_parent);

        // try deflection first
        if (SimulateDeflection(info, normal, collPos, dist, deltaT, ignoreDeflection ? 0.0f : info->MaxDeflectionAngleCos()))
        {
#if _VBS3
          // Check if there exists a event for ammoHit?
          if(ammoType->IsEventHandler(AEAmmoHit) && _real)
          {
            Entity *hitWho = dyn_cast<Entity>(info->object.GetRef());
            OnShotEvent(AEAmmoHit,_parent,hitWho,collPos,_speed,namedComponents,false,normal);  
          }
#endif
          // if the shot was deflected, do not impact
          return;
        }

        // no deflection - simulate penetration if material allows it
        if (SimulatePenetration(info, normal, collPos, dist, deltaT))
        {      
#if _VBS3
          // Check if there exists a event for ammoHit?
          if(ammoType->IsEventHandler(AEAmmoHit) && _real)
          {
            Entity *hitWho = dyn_cast<Entity>(info->object.GetRef());
            OnShotEvent(AEAmmoHit,_parent,hitWho,collPos,_speed,namedComponents,false,normal);  
          }
#endif
          // if the shot penetration was simulated, do not impact
          return;
        }
        
        // no deflection nor penetration -> if the shell is not timed, explode on impact
        if (CanExplodeImmediately())
        {
          if (SimulateHit(info, collPos, normal, VZero))
            _terminate = true;
#if _VBS3
          if(ammoType->IsEventHandler(AEAmmoHit) && _real)
          {
            Entity *hitWho = dyn_cast<Entity>(info->object.GetRef());
            OnShotEvent(AEAmmoHit,_parent,hitWho,collPos,_speed,namedComponents,true,normal);  
          }
          if(ammoType->IsEventHandler(AEAmmoExplode) && _real)
          {
            Entity *hitWho = dyn_cast<Entity>(info->object.GetRef());
            OnShotEvent(AEAmmoExplode,_parent,hitWho,collPos,_speed,namedComponents,true,normal);  
          }
#endif
        }
        FixPosAndSpeedAfterCollision(normal);
      } // if (IsLocal())
      return;
    } //if (minI>=0)
  } //if (numCollisions>0)

  // shot didn't collide with any object, check collision with ground now
  if (groundFarEnough && (groundDist < maxDist))
  {
    Vector3 normal = GLandscape->SurfaceNormal(groundPos);
    float timeDone = groundDist * FutureVisualState()._speed.InvSize();
    // update shot position and speed at the collision
    UpdatePosAndSpeed("landscape", timeDone, groundPos);
    deltaT -= timeDone;
    ///LogF("collision with ground, groundDist=%.4f, groundPos=(%.2f, %.4f, %.2f), normal=(%.2f, %.2f, %.2f)", groundDist, groundPos.X(), groundPos.Y(), groundPos.Z(), normal.X(), normal.Y(), normal.Z());
    // check if the bullet can be deflected instead of exploding
    if (SimulateDeflection(NULL, normal, groundPos, groundDist, deltaT, ignoreDeflection ? 0.0f : 1.0f))
    {
#if _VBS3
      if(ammoType->IsEventHandler(AEAmmoHit))
      {
        AutoArray<RString> noComponents;
        OnShotEvent(AEAmmoHit,_parent,NULL,groundPos,FutureVisualState()._speed,noComponents,false,normal);
      }
#endif
      // if the shot was deflected, do not impact
      return;
    }

    // impact is detected - simulate it
    if (CanExplodeImmediately())
    { // when hitting the ground just explode
#if _VBS3
      // check if we have a event for normal ammo hit
      // if we do, we don't know how many objects we have effected
      // so we just tell the script, we've hit nothing, and our last position
      // was this
      if(ammoType->IsEventHandler(AEAmmoHit) && !hitObject && _real)
      {
        AutoArray<RString> empty;
        OnShotEvent(AEAmmoHit,_parent,NULL,groundPos,_speed,empty);
      }
      if(ammoType->IsEventHandler(AEAmmoExplode) && !hitObject && _real)
      {
        AutoArray<RString> empty;
        OnShotEvent(AEAmmoExplode,_parent,NULL,groundPos,_speed,empty);
      }
#endif
      // TODO: same for missiles; this would allow to simulate not as fast vehicle, but as normal; avoid delete of remote shot
      if (SimulateHit(NULL, groundPos, normal))
      _terminate = true;
    }
    FixPosAndSpeedAfterCollision(normal);
    return;
  } // collided with ground

  // no collision at all
  UpdatePosAndSpeed("free flight", deltaT, newPos);
}

void ShotShell::Simulate(float deltaT, SimulationImportance prec)
{
  if (_terminate)
    // if the shot is already terminated, but not yet deleted, do not simulate it
    return;
  DebugShotShell(NULL, deltaT, FutureVisualState()._speed, FutureVisualState()._speed, "Simulate()");
  PROFILE_SCOPE_EX(simSh, sim);
  _initDelay -= deltaT;
  if (_initDelay <= 0.0f)
  {
    if (_timeToLive < FLT_MAX)
    {
      _timeToLive -= deltaT;
      if (_timeToLive <= 0.0f)
      { // shot lived out
        // avoid delete of remote shot
        _terminate = true;
        return;
      }
    }
    if (_timeToExplosion < FLT_MAX)
    { // timed shot
      _timeToExplosion -= deltaT;
      if (CanExplodeImmediately())
      { // shot should explode now
        // WIP:EXTEVARS:LATER eventually we could check if the shell is lying on the ground and provide corresponding surface normal
        if (SimulateHit(NULL, FutureVisualState().Position()))
        {
#if _VBS3
          if(Type()->IsEventHandler(AEAmmoHit) && _real)
          {
            AutoArray<RString> empty;
            OnShotEvent(AEAmmoHit,_parent,NULL,Position(),_speed,empty);
          }
          if(Type()->IsEventHandler(AEAmmoExplode) && _real)
          {
            AutoArray<RString> empty;
            OnShotEvent(AEAmmoExplode,_parent,NULL,Position(),_speed,empty);
          }
#endif
          _terminate = true;
          return;
        }
      }
    }
    SimulateMovement(deltaT);
  }
#if _VBS2
  MoveAttached(deltaT);
#endif
}


LSError ShotShell::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar));
  CHECK(ar.Serialize("airFriction",_airFriction,0,0));
  CHECK(ar.Serialize("initDelay",_initDelay,0,0));
  CHECK(ar.Serialize("fuseDistanceLeft",_fuseDistanceLeft,0,0));
  CHECK(ar.Serialize("timeToExplosion",_timeToExplosion,0,0));
  return LSOK;
}

void ShotBullet::Simulate(float deltaT, SimulationImportance prec)
{
  base::Simulate(deltaT,prec);
  _tracerStart -= deltaT;
  if(_tracerEnd > 0) //otherwise AAR check in Draw fails!
    _tracerEnd -= deltaT;
}

Object::Visible ShotBullet::VisibleStyle() const
{
  //if (CHECK_DIAG(DECombat)) return false;
  Visible ret = base::VisibleStyle();
  if (ret!=ObjInvisible)
    return ret;
  return !_showTracer || _tracerStart>0 ? ObjInvisible : ObjVisibleFast;
}


//DEFINE_FAST_ALLOCATOR(Missile)
DEFINE_CASTING(Missile)

Missile::Missile(EntityAI *parent, const AmmoType *type, Object *target)
:Shot(parent, type),
_lock(Locked), _engine(Init),
_initTime(type->initTime), _thrustTime(type->thrustTime),
_controlDirectionSet(false),
// _lightColor(0.7,0.8,1.0),
_target(target)
{
  _thrust = type->thrust;
/*
  // TODO: select corresponding cloudlet in config
  if (type->manualControl)
    _cloudlets.Load(Pars >> "CfgCloudlets" >> "CloudletsMissileManual");
  else
    _cloudlets.Load(Pars >> "CfgCloudlets" >> "CloudletsMissile");
*/

  EffectVariables evars(EVarSet_Bullet);
  float intensity = 1.0f;
  float interval = 0.03 * intensity;
  float fireIntensity = 0.5f;
  float fireInterval = 0.03 * fireIntensity;
  float lifeTime = _thrustTime;
  _effects.Init(type->_effectsMissile, this, evars, intensity, interval, fireIntensity, fireInterval, lifeTime);
  if (type->maxControlRange < 1.0f || !target && !type->manualControl )
    // unguided - Lost
    _lock = Lost;
}

void Missile::SetControlDirection(Vector3Par dir)
{
  _controlDirection = dir; // manual missile control
  _controlDirectionSet = true; // manual control activated
}

void Missile::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  if (_thrustTime<=0)
  {
    // TODO: find other method to hide fire trail part
  }
}

void Missile::Simulate(float deltaT, SimulationImportance prec)
{
  PROFILE_SCOPE_EX(simMi, sim);
  const AmmoType *ammoType = Type();
  Vector3Val position = FutureVisualState().Position();
  Vector3Val speed = FutureVisualState().ModelSpeed();
  float mass = GetMass();
  //LogF("Missile %x: %.6f", this, deltaT);

  Vector3 force(VZero), torque(VZero);
  Vector3 friction(VZero), torqueFriction(VZero);
  Vector3 pForce(VZero), pCenter(VZero);
  // body air friction
  pForce[0] = speed[0] * speed[0] * speed[0] * 5e-4f + speed[0] * fabs(speed[0]) * 10.0f + speed[0] * 10.0f;
  pForce[1] = speed[1] * speed[1] * speed[1] * 5e-4f + speed[1] * fabs(speed[1]) * 10.0f + speed[1] * 10.0f;
  pForce[2] = speed[2] * speed[2] * speed[2] * 1e-5f + speed[2] * fabs(speed[2]) * 0.01f + speed[2] * 2.0f;
  pForce *= ammoType->sideAirFriction;
  pForce *= mass * (1.0f / 10.0f);
  friction += pForce;

#if ARROWS
  Vector3 wCenter(VFastTransform, FutureVisualState().ModelToWorld(), GetCenterOfMass());
#endif
  
  bool freeFall = ammoType->thrustTime <= 0.0f;
  if (freeFall)
  {
    // aerodynamic non-stability makes direction aligned with speed
    pForce[0] *= 0.1f;
    pForce[1] *= 0.1f;
    pCenter = Vector3(0.0f, 0.0f, +0.3f);
    torque += pCenter.CrossProduct(pForce);
#if ARROWS
    AddForce(wCenter+FutureVisualState().DirectionModelToWorld(pCenter),FutureVisualState().DirectionModelToWorld(-pForce*InvMass()),Color(1,0,0));
#endif
  }
  else
  {
#if ARROWS
    AddForce(wCenter,FutureVisualState().DirectionModelToWorld(-pForce*InvMass()),Color(1,0,0));
#endif
  }

  if (freeFall)
  {
    // calculate draconic force (which makes direction aligned with speed)
    // note: this should be calculated for all missiles, but it is too late to add it now, as it might cause some changed behavior.
    pForce[0] = speed[0] * fabs(speed[0]) * -0.00033f + speed[0] * -0.005f;
    pForce[1] = speed[1] * fabs(speed[1]) * -0.00033f + speed[1] * -0.005f;
    pForce[2] = 0.0f;
    pForce *= mass;
    force += pForce;
#if ARROWS
    AddForce(FutureVisualState().Position(),pForce*InvMass(),Color(1,1,0));
#endif
  }

  switch (_engine)
  {
    case Init:
      _initTime -= deltaT;
      if (_initTime < 0.0f)
        _engine = (_thrustTime > 0.0f) ? Thrust : Fly;
      break;
    case Thrust:
      {
        _thrustTime -= deltaT;
        if (_thrustTime < 0.0f)
          _engine = Fly;
        else
        {
          // fade in the last part (1/4) of flight
          float fade = _thrustTime * 4.0f / ammoType->thrustTime;
          saturateMin(fade, 1.0f);
          pForce = Vector3(0.0f, 0.0f, _thrust * fade * mass);
          force += pForce;
        }
        // simulate thrusting effects
        _effects.Simulate(deltaT, prec);
      }
      break;
    case Fly:
      break;
  }

  if ((_lock == Locked) && _target && !_target->LockPossible(ammoType))
    // unguided - Lost
    _lock = Lost;

  // parameters of eventual forced explosion
  bool forceExplosion = false;
  Vector3 forcePos = position;
  Vector3 forceNormal = VZero;

  if (_lock == Locked)
  {
    bool cmdDirValid = false;
    Vector3 cmdDir;
    float estT = 0.3f;
    // some missile may be manually controlled
    if (_controlDirectionSet && !_target)
    {
      cmdDir = _controlDirection - position;
      cmdDirValid = true;
    }
    else if (_target)
    {
      Vector3 pos = _target->AimingPosition(_target->FutureVisualState());
      // calculate relative target position
      float dist = pos.Distance(position);
      float estSpeed = ammoType->maxSpeed * 0.3f + speed.Z() * 0.7f;
      float time = dist / floatMax(speed.Z(), estSpeed);
      // lead the target
      pos += ammoType->trackLead * time * _target->ObjectSpeed();
      /*
      if (freeFall)
      {
        // for free falling bombs, aim slightly above the target
        // having no propulsion, we need this to avoid hitting ground too soon
        pos[1] += pos.Distance(position)*0.035f;
      }
      */
      cmdDir = pos - position;
      cmdDirValid = true;
      estT = floatMin(0.3f, time);
    }

    if (cmdDirValid)
    {
      // estimate orientation change with current angular properties
      // adjust orientation so that rPos and rSpeed is aligned
      Matrix3 estOrientation = FutureVisualState().Orientation();
      // convert to estimated model space
      Matrix3Val invEstOrientation = estOrientation.InverseRotation();
      // calculate estimated relative position and speed
      Vector3Val rSpeed = invEstOrientation * (FutureVisualState()._speed + estT * FutureVisualState()._acceleration);
      Vector3Val rPos = invEstOrientation * cmdDir;
      // with more maneuverable ammo we assume speed rather then orientation
      float dFactor = ammoType->maneuvrability * 0.3f;
      saturate(dFactor, 0.5f, 0.95f);
      Vector3 rDir = rSpeed.Normalized() * dFactor + VForward * (1.0f - dFactor);

      Vector3 rdn = rDir.Normalized();
      Vector3 rpn = rPos.Normalized();

      // rSpeed should be near VForward
      float factor = 20.0f * ammoType->trackOversteer;
      float up = (rpn.Y() - rdn.Y()) * factor;
      float left = (rpn.X() - rdn.X()) * factor;

      // control up/down flaps
      //float up=pos.Y()*invZ*10;
      //float left=pos.X()*invZ*10;
      if (speed.Z() < 30.0f)
        up = left = 0.0f; // disable controls when flying slow

      // check if we can maintain lock
      bool lostControl = false;
      if (ammoType->manualControl)
        // controlling vehicle is no longer able to control or is too far
        lostControl = !_parent || !_parent->CanFire() || _parent->FutureVisualState().Position().Distance2(position) >= Square(ammoType->maxControlRange);
      else if (_target)
      { // target is behind us or out of visible cone (for fire and forget weapon)
        Vector3 relPos = PositionWorldToModel(_target->FutureVisualState().Position());
        float coneCheck = relPos.Z() * 1.5f;
        lostControl = (relPos.Z() < 0.0f) || (fabs(relPos.X()) > coneCheck) || (fabs(relPos.Y()) > coneCheck);
      }
      if (lostControl)
      {
        // nasty trick to stop rotation (once only)
        _angMomentum = VZero;
        up = left = 0.0f;
        _lock = Lost;
      }

      //if( fabs(up)>5 || fabs(left)>5 ) _lock=Lost;
      float turn = speed.Z() * (1.0f / 50.0f);
      saturate(turn, 0.1f, 3.0f);
      float invTurn = 3.0f / turn;
      // correct for slow speed
      up *= invTurn;
      left *= invTurn;

      if (CHECK_DIAG(DECombat) && GWorld->CameraOn() == this)
        DIAG_MESSAGE(500, Format("Lock cmd %.1f,%.1f,%.1f, rDir %.2f,%.2f,%.2f, rPos %.2f,%.2f,%.2f, control %.2f,%.2f",
          cmdDir.X(),cmdDir.Y(),cmdDir.Z(),rdn.X(),rdn.Y(),rdn.Z(),rpn.X(),rpn.Y(),rpn.Z(),left,up));
      float maxMan = ammoType->maneuvrability * 0.25f;
      //if (freeFall) maxMan *= 0.333f;
      saturate(up, -maxMan, +maxMan);
      saturate(left, -maxMan, +maxMan);
      
      Vector3 pCenter = Vector3(0.0f, 0.0f, ammoType->maneuvrability * 0.04f);
      Vector3 pForce = mass * turn * Vector3(left, up, 0.0f);
      torque += pCenter.CrossProduct(pForce);

      if (freeFall)
        // assume that besides of torque, controls cause some force
        force += pForce;

#if ARROWS
      AddForce(FutureVisualState().PositionModelToWorld(pCenter), FutureVisualState().DirectionModelToWorld(pForce*InvMass()), Color(0,0,1));
#endif
    } // if (cmdDirValid)
  } //if (_lock == Locked)
  else if (_lock == Lost)
  {
    // we are lost - do proximity based touch off (not for free fall bombs, only active ammo can do this)
    // check if we can maintain lock (for fire and forget weapon)
    if (_target && !freeFall && ammoType->maxControlRange>0)
    {
      // calculate relative target position
      Vector3 relPos = PositionWorldToModel(_target->FutureVisualState().Position());
      if (relPos.Z() < 0.0f && relPos.SquareSize() < Square(20.0f))
        // target is behind us - we have missed, but we are still close - go off
        forceExplosion = true;
    }
  }

  // convert to world space
  FutureVisualState().DirectionModelToWorld(friction, friction);
  FutureVisualState().DirectionModelToWorld(force, force);
  FutureVisualState().DirectionModelToWorld(torque, torque);

  torqueFriction = _angMomentum * 5.0f;

  // add gravity
  pForce = Vector3(0.0f, -G_CONST, 0.0f) * mass;
  force += pForce;

  // calculate new position
  VisualState moveTrans = PredictPos<VisualState>(deltaT);

  ApplyRemoteStateAdjustSpeed(deltaT,moveTrans);

  Vector3Val newPos = moveTrans.Position();
  Vector3 lDir = newPos - position;

  if ((deltaT > 0.0f) && (lDir.SquareSize() > 0.0f) && IsLocal()
#if _EXT_CTRL
    && !IsExternalControlled()
#endif
    )
  {
    float maxDist = lDir.Size();
    // calculate intersection with land between current and new position
    Vector3 groundPos;
    float groundDist = GLandscape->IntersectWithGroundOrSea(&groundPos, position, lDir.Normalized(), 0.0f, maxDist);

    // test collisions with objects
    CollisionBuffer collision;
    // FIXME: ignore _parent only for a limited amount of time?
    GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),collision, Landscape::FilterIgnoreTwo(this, _parent), position, newPos, 0.0f);
    int numCollisions = collision.Size();
    if (numCollisions > 0)
    {
      // find time and index of the nearest collision
      float minT = FLT_MAX;
      int minI = -1;
      for (int i = 0; i < numCollisions; i++)
      {
        const CollisionInfo &info = collision[i];
        if (!info.object)
          continue;
        float t = info.under;
        // check if ground collision was earlier
        if (t * maxDist >= groundDist)
          continue;
        // pass through easily penetrable obstacles
        if (CanPenetrateFreely(info.BulletPenetrability()))
          continue;
        if (t < minT)
        {
          minT = t;
          minI = i;
        }
      }

      // handle collision with the object
      if (minI >= 0)
      {
        const CollisionInfo &info = collision[minI];

        Object *hObj = info.object;
        EntityAI *ent = dyn_cast<EntityAI>(hObj);

        if (ent) ent->OnHitPart(info,Type(),_parent);

        // explode (missile is always assumed to have a full energy)
        Vector3Val normal = info.dirOutNotNorm.Normalized();
        HitInfo hitInfo(ammoType, info.pos, normal, FutureVisualState()._speed);
        Object *hitObject = info.HitObject();
        GLandscape->ExplosionDamage(_parent, OriginDirection(), hitObject, ModelPosFromDirectHit(hitObject, hitInfo._pos), hitInfo, info.object, info.component);
#if _VBS3
        // Check if there exists a event for ammoHit?
        if(ammoType->IsEventHandler(AEAmmoExplode))
        {
          Entity *hitWho = dyn_cast<Entity>(info.object.GetRef());
          OnShotEvent(AEAmmoExplode, _parent, hitWho, info.pos, FutureVisualState()._speed, namedComponents, true, info.dirOutNotNorm.Normalized());
        }
#endif
        SetDeleteRaw();
#if _ENABLE_CHEATS
        if ((GWorld->CameraOn() == this) && _parent)
          GWorld->SwitchCameraTo(_parent, CamInternal, true);
#endif
        return;
      }
    }

    // missile didn't collide with any object, check collision with ground now
    if (groundDist < maxDist)
    {
#if _VBS3
      // not object collision, we're reacting to ground collision
      if(ammoType->IsEventHandler(AEAmmoHit))
      {
        AutoArray<RString> empty;
        OnShotEvent(AEAmmoHit, _parent, NULL, groundPos, FutureVisualState()._speed, empty, true);
      }
#endif
      // force explosion
      forceExplosion = true;
      forcePos = groundPos;
      forceNormal = GLandscape->SurfaceNormal(groundPos);
    }
  }
  _timeToLive -= deltaT;

  if (IsLocal()
#if _EXT_CTRL
    && !IsExternalControlled()
#endif
    )
  {
    if (_timeToLive < 0.0f)
      forceExplosion = true;

    if (forceExplosion)
    { // force explosion
#if _VBS3
      if(ammoType->IsEventHandler(AEAmmoExplode))
      {
        AutoArray<RString> empty;
        OnShotEvent(AEAmmoExplode, _parent, NULL, forcePos, FutureVisualState()._speed, empty);
      }
#endif
      HitInfo hitInfo(ammoType, forcePos, forceNormal, FutureVisualState()._speed);
      GLandscape->ExplosionDamage(_parent, OriginDirection(), NULL, VZero, hitInfo);
      SetDeleteRaw();
#if _ENABLE_CHEATS
      if (this == GWorld->CameraOn() && _parent)
        GWorld->SwitchCameraTo(_parent, CamInternal, true);
#endif
      return;
    }
  }

  Move(moveTrans);

  DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
  ApplyForces(deltaT, force, torque, friction, torqueFriction);

#if _VBS3
  MoveAttached(deltaT);
#endif
  /*
  LogF("missile: force %.1f,%.1f,%.1f, friction %.1f,%.1f,%.1f, speed %.1f,%.1f,%.1f",
    force[0],force[1],force[2],friction[0],friction[1],friction[2],_modelSpeed[0],_modelSpeed[1],_modelSpeed[2]);
  */
}

void Missile::Sound(bool inside, float deltaT)
{
  PROFILE_SCOPE_EX(snMis,sound);
  const SoundPars &sound=Type()->_soundEngine;
  if (_engine == Thrust)
  {
    const VisualState &vs = RenderVisualState();
    if (!_soundEngine && sound.name.GetLength() > 0)
      _soundEngine = GSoundScene->OpenAndPlay(sound.name, NULL, false, vs.Position(), vs.Speed(), sound.vol, sound.freq, sound.distance);
    if (_soundEngine)
    {
      float coef = _thrust * (1.0f / 800.0f);
      float obstruction = 1.0f;
      float occlusion = 1.0f;
      GWorld->CheckSoundObstruction(NULL,false,obstruction,occlusion);
      _soundEngine->SetObstruction(obstruction,occlusion,deltaT);
      _soundEngine->SetVolume(sound.vol*coef,sound.freq);
      _soundEngine->SetPosition(vs.Position(),vs.Speed());
    }
  }
  else
    _soundEngine.Free();
  base::Sound(inside,deltaT);
}

void Missile::UnloadSound()
{
  _soundEngine.Free();
  base::UnloadSound();
}

DEFINE_CASTING(Flare)

Flare::Flare(EntityAI *parent, const AmmoType *type)
: base(parent,type, true),
_lightColor(1.0, 1.0, 1.0)
{
  _airFriction = type->_airFriction;
  const ParamEntry &cls = *type->_par;
  _lightColor = GetColor(cls >> "lightColor");
  _radius = cls >> "radius";
  _size = cls >> "size";
}

void Flare::Simulate(float deltaT, SimulationImportance prec)
{
  base::Simulate(deltaT, prec);
  const float ill_expl = 2.0f;
  if (!_light && (_timeToLive <= Type()->_timeToLive - ill_expl))
  {
    _light = new LightPointOnVehicle(GLOB_SCENE->Preloaded(SphereLight), _lightColor, Color(HBlack), this, Vector3(0, 0, -0.5));
    _light->SetRadius(_radius);
#if _VBS3_FLAREFIX
    _light->SetSize(10000.0f);
#else
    _light->SetSize(_size);
#endif
    GLOB_SCENE->AddLight(_light);

    _airFriction = -0.2;
    
    // run script
    RString name = RString("onFlare.sqs");
    if (GetMissionDirectory().GetLength() > 0 && QFBankQueryFunctions::FileExists(GetMissionDirectory() + name))
    {
      GameArrayType color;
      color.Add(_lightColor.R());
      color.Add(_lightColor.G());
      color.Add(_lightColor.B());
      GameArrayType arguments;
      arguments.Add(color);
      arguments.Add(GameValueExt(_parent,GameValExtObject));
      
      Script *script = new Script(name, arguments, GWorld->GetMissionNamespace()); // mission namespace
      GWorld->AddScript(script, false);
      /// the script will be simulated in nearest World::Simulate - this is enough
    }
  }
  if (_light)
  {
    float fade = GRandGen.PlusMinus(0.8f, 0.2f);
    _light->SetDiffuse(_lightColor * fade);
  }
}

#define ILLUMINATING_SHELL_MSG_LIST(XX) \
  XX(Create, CreateShot) \
  XX(UpdatePosition, UpdatePositionVehicle)

DEFINE_NETWORK_OBJECT(Flare, base, ILLUMINATING_SHELL_MSG_LIST)

LSError Flare::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(::Serialize(ar, "lightColor", _lightColor, 1))
  return LSOK;
}

DEFINE_CASTING(MarkerLightShell)

MarkerLightShell::MarkerLightShell(EntityAI *parent, const AmmoType *type)
: base(parent,type, true)
{
  _activation = Glob.time;
}

bool MarkerLightShell::Init()
{
  // create enemy
  if (IsLocal())
  {
    const char *laserName = "NVG_TargetC";

    if (_parent)
    {
      TargetSide side = _parent->GetTargetSide();
      Person *person = dyn_cast<Person, EntityAI>(_parent);
      if (person && person->Brain()) side = person->Brain()->GetSide();

      if (side == TEast) laserName = "NVG_TargetE";
      else if (side == TWest) laserName = "NVG_TargetW";
    }

    if (_nvTarget.IsNull())
    {
      _nvTarget = GWorld->NewVehicleWithID(laserName,"");

      if (_nvTarget)
      {
        _activation = Glob.time;

        Matrix4 origin;
        origin.SetDirectionAndUp(VForwardP, VUpP);
        origin.SetPosition(FutureVisualState().Position());

        _nvTarget->SetTransform(origin);
        GWorld->AddVehicle(_nvTarget);
        
        if (GWorld->GetMode() == GModeNetware)
          GetNetworkManager().CreateVehicle(_nvTarget, VLTVehicle, "", -1);
      }

      return true;
    }
  }  

  return false;
}

bool MarkerLightShell::SimulateHit(const CollisionInfo *info, const Vector3P &pos, const Vector3P &surfNormal, const Vector3P &newSpeed)
{
  return Init();
}

void MarkerLightShell::Simulate(float deltaT, SimulationImportance prec)
{
  if (IsLocal())
  {
    if (_nvTarget.NotNull() && FutureVisualState().Speed().SquareSize() > 0.1f)
    {
      _nvTarget->Move(FutureVisualState().Position());
    }

    const float InitializationTime = 5.0f;

    // auto activation
    if (_activation < Glob.time - InitializationTime)
    {
      if (_nvTarget.IsNull()) Init();
    }

    if (_activation < Glob.time - (InitializationTime * 2.0f))
    {
      SetDelete();
    }
  }

  base::Simulate(deltaT, prec);
}

#define MARKERLIGHT_SHELL_MSG_LIST(XX) \
  XX(Create, CreateShot) \
  XX(UpdatePosition, UpdatePositionVehicle)

DEFINE_NETWORK_OBJECT(MarkerLightShell, base, MARKERLIGHT_SHELL_MSG_LIST)

LSError MarkerLightShell::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeRef("nvTarget", _nvTarget, 1))
  CHECK(::Serialize(ar, "activation", _activation, 1, TIME_MIN))
  CHECK(base::Serialize(ar))
  return LSOK;
}

DEFINE_CASTING(SmokeShell)

SmokeShell::SmokeShell(EntityAI *parent, const AmmoType *type)
: base(parent, type, true)
{
  _airFriction = type->_airFriction;
	_smoking = false;

  // load smoke parameters
  const ParamEntry &cls = *type->_par;

  ParamEntryVal smokeColorArray = cls >> "smokeColor";
  _smokeColor[0]= smokeColorArray[0];
  _smokeColor[1]= smokeColorArray[1];
  _smokeColor[2]= smokeColorArray[2];
  _smokeColor[3]= smokeColorArray[3];

  _effects.Init(Pars >> type->_effectsSmoke, this, EVarSet_SmokeShell);
}

void SmokeShell::Simulate(float deltaT, SimulationImportance prec)
{
  // base simulation does everything we need, including deflection
  base::Simulate(deltaT, prec);
  if (_smoking)
    _effects.Simulate(deltaT, prec, _smokeColor, lenof(_smokeColor));
}

LSError SmokeShell::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("smoking",_smoking,0,true));
  CHECK(base::Serialize(ar))
  return LSOK;
}


DEFINE_FAST_ALLOCATOR(ShotBullet)
DEFINE_CASTING(ShotBullet)

ShotBullet::ShotBullet(EntityAI *parent, const AmmoType *type, bool real, bool showTracer):ShotShell(parent,type,real)
{
  _timeToLive = type->_timeToLive;
  
  _posSuppressTraced = VZero;
  
  // TODO: for silenced weapon bullets we never want to trace any suppression
  _suppressionDistanceLeft = 150.0f;

  _beg = VZero;
  _nSegs = 0;

  _tracerStart = Type()->_tracerStartTime * (1 + GRandGen.PlusMinus(0,.05)); // +/- 5% variation
  _tracerEnd = Type()->_tracerEndTime * (1 + GRandGen.PlusMinus(0,.05));
  _muzzlePos = VZero; //set to the fist _beg

  // If tracerStart is negative, we don't draw tracers
  showTracer &= (_tracerStart >= 0);

  _showTracer = showTracer;
#if 0
  _shape = GScene->Preloaded(BulletLine);
  float width=type->hit*(1.0/20);
  saturate(width,0.1,2);

  // get tracer color

  //;
  //PackedColor color(Color(1,1,0.3,width));
  PackedColor color = type->_tracerColor;
  if (!Glob.config.IsEnabled(DTTracers))
  {
    color = type->_tracerColorR;
  }
  float a8 = color.A8()*width;
  saturate(a8,0,255);
  color.SetA8(toInt(a8));
  SetConstantColor(color);
#endif
}

void ShotBullet::SetBeg(Vector3Val beg)
{
  _beg = beg;
  if(_muzzlePos == VZero)
    _muzzlePos = beg; //store 
}

void ShotBullet::SetEnd(Vector3Val end)
{
  if (_beg.SquareSize() <= 1e-6f)
    _beg = end;
  if (_nSegs < MaxSegs)
    _nSegs++;
  _segEnd[_nSegs-1] = end;
}

float ShotBullet::TracerLength(ObjectVisualState const& vs) const
{
  return vs.cast<Entity>()._speed.Size() * (0.02f + 0.03f * GScene->MainLight()->NightEffect());
}

/// called once per frame for each fast vehicle
/**
Simulate is typically called several times per frame.
*/
void ShotBullet::StartFrame()
{
  if (_posSuppressTraced == VZero)
    _posSuppressTraced = FutureVisualState().Position();
  _nSegs = 0;
  SetBeg(FutureVisualState().Position());
  base::StartFrame();
}

void ShotBullet::EndFrame()
{
  TerminateSegment(false);
}

void ShotBullet::TerminateSegment(bool stopSuppression)
{
  if (_parent && _suppressionDistanceLeft > 0.0f)
  {
    DoAssert(_posSuppressTraced.SquareSize() > 0.0f);
    // we want the part beg ... end to be rasterized if needed
    Vector3 toTrace = FutureVisualState().Position()-_posSuppressTraced;
    float toTraceSize2 = toTrace.SquareSize();
    if (toTraceSize2 > 0.0f)
    {
      float toTraceInvSize = InvSqrt(toTraceSize2);
      float distSegment = toTraceSize2*toTraceInvSize;
      
      // rasterize the segment, or a part of it
      if (distSegment > _suppressionDistanceLeft)
      {
        // segment too long - rasterize only a part of it
        Vector3 end = _posSuppressTraced + toTrace*(toTraceInvSize*_suppressionDistanceLeft);
        GLandscape->SuppressCache()->Trace(_parent,Type()->hit,_suppressionDistanceLeft,_posSuppressTraced,end);
        _suppressionDistanceLeft = 0.0f;
        _posSuppressTraced = end;
      }
      else
      {
        // rasterize whole segment
        GLandscape->SuppressCache()->Trace(_parent,Type()->hit,_suppressionDistanceLeft,_posSuppressTraced,FutureVisualState().Position());
        _suppressionDistanceLeft -= distSegment;
        _posSuppressTraced = FutureVisualState().Position();
      }
    }
    if (stopSuppression)
      // no more suppression tracing after this segment
      _suppressionDistanceLeft = 0.0f;
  }
  SetEnd(FutureVisualState().Position());
}

float ShotBullet::EstimateArea() const
{
  return TracerLength(RenderVisualState());
}

float ShotBullet::GetCollisionRadius(const ObjectVisualState &vs) const
{
	Matrix4 worldToModel = vs.GetInvTransform();
	Vector3 beg = worldToModel.FastTransform(_beg);
	float maxRadius2 = beg.SquareSize();
	Vector3 end = beg-VForward*TracerLength(vs);
	float radius2 = end.SquareSize();
	if (maxRadius2<radius2) maxRadius2 = radius2;
	return sqrt(maxRadius2);
}

float ShotBullet::ClippingInfo(Vector3 *minMax, ClippingType clip) const
{
  Matrix4 worldToModel = RenderVisualState().GetInvTransform();
  Vector3 beg = worldToModel.FastTransform(_beg);
  minMax[0] = beg, minMax[1] = beg;
  float maxRadius2 = beg.SquareSize();
  Vector3 end = beg-VForward*TracerLength(RenderVisualState());
  CheckMinMax(minMax[0],minMax[1],end);
  float radius2 = end.SquareSize();
  if (maxRadius2<radius2) maxRadius2 = radius2;
  return sqrt(maxRadius2);
}

void ShotBullet::AnimatedBSphere(int level, Vector3 &bCenter, float &bRadius, Matrix4Par pos, float posMaxScale) const
{
  if (_nSegs!=1)
  {
    Vector3 minMax[2];
    minMax[0] = minMax[1] = _beg;
    for (int i=0; i<_nSegs; i++)
      CheckMinMax(minMax[0],minMax[1],_segEnd[i]);
    
    bCenter = (minMax[0]+minMax[1])*0.5f;
    bRadius = minMax[0].Distance(minMax[1])*0.5f;
  }
  else
  {
    bCenter = (_beg+_segEnd[0])*0.5f;
    bRadius = _beg.Distance(_segEnd[0])*0.5f;
  }
}


void ShotBullet::Draw(int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
  const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
  , int drawListId
#endif
  )
{
  bool showTracer = _showTracer || CHECK_DIAG(DECombat);
  if(showTracer && _nSegs>0 && _tracerStart < 0 && (_tracerEnd > 0 || _tracerEnd == -1)) //-1 is for AAR
  {
    // NV tracers are visible in TI
    if (!GEngine->GetThermalVision() && Type()->_nvgOnly && !GEngine->GetNightVision() && !CHECK_DIAG(DECombat)) //don't display shot if it's only NVG visible
      return;

    if (_shape)
    {
      if(_nSegs > 1)
        _beg = _segEnd[_nSegs - 2];
      Vector3 dir = _segEnd[_nSegs - 1] - _beg;

      if (dir.SquareSize()<=FLT_MIN) dir = RenderVisualState().Direction();
      dir.Normalize();
      // avoid creating a singular matrix
      FutureVisualStateRaw().SetOrient(dir.Normalized(), (dir.Distance2(VUp) > 1e-3f) ? VUp : VForward);  // bypass ProtectedVisualState check

      /* if(GScene->GetCamera()->Direction().CosAngle(Direction()) < -0.93969f) //ignore if bullets are within a 20deg Angle towards the camera
        return; */

      float night = GScene->MainLight()->NightEffect(); //make tracers longer at night time
      float speed = RenderVisualState()._speed.Size();
      float length = TracerLength(RenderVisualState()); //base length on speed rather than FPS /todo normalize model!
      float distFromMuzzle = Vector3(_segEnd[_nSegs - 1] - _muzzlePos).Size();

      if(distFromMuzzle < Type()->_tracerStartTime * speed) //tracer not shown yet
        return;

      float maxLength = distFromMuzzle * .6;

      saturateMin(length, 100);
      saturate(length, 1, maxLength); 

      float dist2 = GScene->GetCamera()->Position().Distance(RenderVisualState().Position()) * (0.035 + (0.02 * night)); 

      saturate(dist2,1,50); 
      if(GEngine->GetNightVision()) //scale if night vision is on
        dist2 *= 3.;

      Object *cameraOn = GWorld->CameraOn();
      if (!cameraOn)
        return;

      float fov = cameraOn->GetCameraFOV(); //adjust to FOV
      dist2 *= fov * Type()->_tracerScale;            //scale is set in Config

      // make tracers longer and thicker depending on distance to cam and speed
      Matrix4Val oldPos = pos.position;
      Matrix4 newPos;
      // we do not care what coordinate space we are in - we just transform it
      newPos.SetOrientation(oldPos.Orientation() * Matrix3(MScale, dist2, dist2,length));
      newPos.SetPosition(oldPos.Position());
      const float alphaCoef = 500.0f;
      float alpha = alphaCoef / (dist2 * dist2 * length);
      saturate(alpha, 0.0f, 1.0f);
      GEngine->SetInstanceInfo(cb,Color(1,1,1,alpha), 1);

#if _HELISIM_RENDER_TO_TEXTURE
      base::Draw(cb,level, clipFlags, dp, ip, dist2, PositionRender(newPos,pos.camSpace), oi, drawListId);
#else
      base::Draw(cb,level, clipFlags, dp, ip, dist2, PositionRender(newPos,pos.camSpace), oi);
#endif
      
      GEngine->SetInstanceInfo(cb,HWhite, 1);
    }
    /*
    else
    {
      // TODO: if there is no shape, render diagnostics / arcade tracers
      DoDraw(level, clipFlags, pos);
    }
    */
  }
}

DEFINE_FAST_ALLOCATOR(ShotSpread)
DEFINE_CASTING(ShotSpread)

#define DEG2RAD (H_PI/180)

ShotSpread::ShotSpread(EntityAI *parent, const AmmoType *type, const WeaponModePtr *ptr, bool real): base(parent, type, real, false)
{
  if (ptr)
  {
    const MuzzleType *muzzle = ptr->GetMuzzleType();
    if (muzzle) _spreadAngle = muzzle->_shotSpreadAngle;
  }
}

SmallShell::SmallShell(): _active(true) {}
SmallShell::SmallShell(const SmallShell &par): _active(par._active), _pos(par._pos), _speed(par._speed) {}
SmallShell::SmallShell(Vector3Par pos, Vector3Par speed): _pos(pos), _speed(speed), _active(true) {}

void ShotSpread::Simulate(float deltaT, SimulationImportance prec)
{
  if (_terminate)
    // if the shot is already terminated, but not yet deleted, do not simulate it
    return;

  if (_timeToLive < FLT_MAX)
  {
    _timeToLive -= deltaT;
    if (_timeToLive <= 0.0f)
    { // shot lived out
      // avoid delete of remote shot
      _terminate = true;
      return;
    }
  }

  if (_shells.Size() == 0)
    CreateSmallShels();

  SimulateMovement(deltaT);
}

void ShotSpread::CreateSmallShels()
{
  //TODO create proper shells with random spread

  SmallShell tmp(FutureVisualState().Position(), FutureVisualState()._speed);
  _shells.Append(tmp);

  // first circle
  for (int i=0; i<4; ++i)
  {
    float curAngle = _spreadAngle/4.0f + GRandGen.RandomValue()*_spreadAngle/4.0f;

    Matrix3 mat;
    mat.SetIdentity();
    mat.SetRotationY( curAngle * DEG2RAD);
    tmp._speed = mat * FutureVisualState()._speed;

    mat.SetIdentity();
    mat.SetRotationAxis(FutureVisualState()._speed, (i * 90 + 45 * GRandGen.RandomValue() ) * DEG2RAD);
    tmp._speed = mat * tmp._speed;
    _shells.Append(tmp);
  }

  // second circle
  for (int i=0; i<6; ++i)
  {
    float curAngle = _spreadAngle*3.0f/4.0f  + GRandGen.RandomValue()*_spreadAngle/4.0f;

    Matrix3 mat;
    mat.SetIdentity();
    mat.SetRotationY( curAngle * DEG2RAD);
    tmp._speed = mat * FutureVisualState()._speed;

    mat.SetIdentity();
    mat.SetRotationAxis(FutureVisualState()._speed, (i * 60 + 40 * GRandGen.RandomValue() ) * DEG2RAD);
    tmp._speed = mat * tmp._speed;
    _shells.Append(tmp);
  }

}

void ShotSpread::SimulateMovement(float deltaT)
{
  if (deltaT <= 0.0f)
    return;

  const AmmoType *ammoType = Type();
  RString ammo = ammoType->ParClass().GetName();
  bool anyActive = false;

  // move and calculate all shells
  for (int i=0;i<_shells.Size();++i)
  {
    SmallShell &shell = _shells[i];

    // check if shell is active
    if (!shell._active)
      continue;

    anyActive = true;

    // update speed of the shell
    Vector3Val accel = shell._speed * (shell._speed.Size() * _airFriction);
    Vector3 grav = VUp * -G_CONST * Type()->_coefGravity;
    shell._speed += (accel + grav) * deltaT;

    // calculate shell's new position
    Vector3 origPos = shell._pos;
    Vector3 lDir = shell._speed * deltaT;
    Vector3 newPos = origPos + lDir;
    float maxDist = shell._speed.Size() * deltaT;

    // calculate intersection with land between current and new position
    Vector3 groundPos;
    bool collidingWithWater;
    float groundDist = GLandscape->IntersectWithGroundOrSea(&groundPos, collidingWithWater, origPos, shell._speed.Normalized(), -maxDist, maxDist);
    bool groundFarEnough = groundDist >= 0.1f;

    CollisionBuffer collision;
    GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),collision, Landscape::FilterIgnoreTwo(this, _parent), origPos, newPos, 0.0f);
    int numCollisions = collision.Size();
    if (numCollisions > 0)
    {
      shell._active = false;
      float minDist = FLT_MAX;
      int minI = -1;

      // shot bullets can go through glass
      // FIXME: why only bullets, what about other shots (like grenades, sabots, etc.)?
      bool detectGlass = dyn_cast<ShotSpread>(this) != NULL;
      const Texture *glass = GPreloadedTextures.New(TextureBlack);

      // find nearest colliding object
      for (int i = 0; i < numCollisions; i++)
      {
        const CollisionInfo &info = collision[i];
        if (!info.object)
          continue;
        float dist = info.under * maxDist;
        // check if collision with the ground is earlier
        if (groundFarEnough && (dist >= groundDist))
          continue;
        if (detectGlass && (info.texture == glass))
          continue;
        if(info.Thickness()>0 && !info.entry)
          continue;
        if (dist < minDist)
        {
          minDist = dist;
          minI = i;
        }
      }  

      // damage all glass objects
      if (CanDamage())
      { // damage what was hit; note that the accumulated damage is applied when the dmgAcc is destroyed at the end of this scope
        // TODO: damage the parent object when the proxy was hit
        DoDamageAccumulator dmgAcc;
        for (int i = 0; i < numCollisions; i++)
        {
          CollisionInfo &info = collision[i];
          if (!info.object)
            continue;
          float dist = info.under * maxDist;
          if (groundFarEnough && (dist >= groundDist))
            continue;
          // stop on first solid obstacle
          if (detectGlass && (info.texture != glass))
            continue;
          if (dist > minDist)
            continue;
          // damage corresponding component
          DoDamageResult result;
          Object* dest = info.object;
          dest->DirectLocalHit(result, info.component, ammoType->hit);
          
          SkinnedPosition pos = CollisionSkinning(info);
          dmgAcc.Add(dest, _parent, ammo, pos, result);
        }
      }

      if (minI >= 0)
      {
        const CollisionInfo *info = &collision[minI];
        shell._pos = info->pos;
        SimulateHit(info, shell._pos);
      }
    }
    else
    {
      // did shell hit the ground (or will it hit ground in next step)?
      if (groundDist < maxDist)
      {
        Vector3 normal = GLandscape->SurfaceNormal(groundPos);
        SimulateHit(NULL, groundPos, normal);
        shell._pos = groundPos;
        shell._active = false;
        continue;
      }

      shell._pos = newPos;
    }
  }

  // destroy shot if none of the shells are active
  if (!anyActive)
    _terminate = true;
}

void ShotSpread::DrawDiags()
{
#if _ENABLE_CHEATS
  /*if (CHECK_DIAG(DEShots))
  {
    for(int i=0;i<_shells.Size();++i)
    {
      const SmallShell &shell = _shells[i];
      GScene->DrawDiagArrow(shell._pos - shell._speed.Normalized(), shell._speed, 1.0, (shell._active? ColorP(0.0, 1.0, 0.0) : ColorP(1.0, 0.0, 0.0)));
    }
  }*/
#endif
}

DEFINE_CASTING(CounterMessure)

CounterMessure::CounterMessure(EntityAI *parent, const AmmoType *type)
: base(parent,type, true)
{
  _smoking = true;
  _airFriction = type->_airFriction;
  _thrustTime = type->thrustTime;

  _maxControlRange = type->maxControlRange;
  _initTime = type->initTime;

  _startTime = Glob.time.toFloat();
  _effects.Init(Pars >> type->_effectsSmoke, this, EVarSet_CounterMessure);
}

void CounterMessure::SimulateCMMovement(float deltaT)
{
  Vector3 origPos = FutureVisualState().Position();
  Vector3 lDir = FutureVisualState()._speed * deltaT;
  Vector3 newPos = origPos + lDir;
  float maxDist = FutureVisualState()._speed.Size() * deltaT;

  if (maxDist <= 0.0f)
  { // even at very slow movement, gravitation must still be handled
    UpdatePosAndSpeed("no speed", deltaT, newPos);
    return;
  }

  Vector3 collisionBeg = origPos;
  const float nearCollisionDistance = 0.1f;
  collisionBeg += FutureVisualState()._speed.Normalized() * nearCollisionDistance;

  // calculate intersection with land between current and new position
  Vector3 groundPos;
  bool collidingWithWater;
  float groundDist = GLandscape->IntersectWithGroundOrSea(&groundPos, collidingWithWater, origPos, FutureVisualState()._speed.Normalized(), 0.0f, maxDist);
  bool groundFarEnough = false || (groundDist >= nearCollisionDistance);

  // shot didn't collide with any object, check collision with ground now
  if (groundFarEnough && (groundDist < maxDist))
  {
    Vector3 normal = GLandscape->SurfaceNormal(groundPos);
    float timeDone = groundDist * FutureVisualState()._speed.InvSize();
    // update shot position and speed at the collision
    UpdatePosAndSpeed("landscape", timeDone, groundPos);
    deltaT -= timeDone;
    ///LogF("collision with ground, groundDist=%.4f, groundPos=(%.2f, %.4f, %.2f), normal=(%.2f, %.2f, %.2f)", groundDist, groundPos.X(), groundPos.Y(), groundPos.Z(), normal.X(), normal.Y(), normal.Z());
    // check if the bullet can be deflected instead of exploding
    if (SimulateDeflection(NULL, normal, groundPos, groundDist, deltaT, 1.0f))
    {
      // if the shot was deflected, do not impact
      return;
    }

    FixPosAndSpeedAfterCollision(normal);
    return;
  } // collided with ground
  // no collision at all
  UpdatePosAndSpeed("free flight", deltaT, newPos);
}

void CounterMessure::Simulate(float deltaT, SimulationImportance prec)
{
  _timeToLive -= deltaT;
  _thrustTime -= deltaT;
  Shot::Simulate(deltaT, prec);
  if(_timeToLive <0.0f)
  {
    SetDeleteRaw();
    return;
  }
  else
  {
    //GScene->DrawDiagSphere(Position(), 1.0f, PackedColor(100,255,100,toInt(1 * 255)));
    if (_smoking)
    {
      float params[] = {Glob.time.toFloat() - _startTime}; 
      _effects.Simulate(deltaT, prec, params, lenof(params));
    }
    SimulateCMMovement(deltaT);
  }
}

#define COUNTERMMESSURE_SHELL_MSG_LIST(XX) \
  XX(Create, CreateShot) \
  XX(UpdatePosition, UpdatePositionVehicle)

DEFINE_NETWORK_OBJECT(CounterMessure, base, COUNTERMMESSURE_SHELL_MSG_LIST)

LSError CounterMessure::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("smoking",_smoking,0,true));
  return LSOK;
}


#if !_RELEASE
  #define ARROWS 1
#endif

#if _VBS2 // suppression
// helper function for solving time of flight equations
static float SolveExpansion(float target, float friction, float frameRate) 
{
  float limit = 1.0/friction;
  if (limit<target)
    return -1;
  float term = 1.0;
  float series = term;
  float accSeries = 0;
  int count = 1;
  double fraction = 0;
  while (series<target) 
  {
    term *= (1.0-friction);
    count++;
    series+=term;
    accSeries += (series-1);
  }
  if (series>target) 
  {
    accSeries -=series;
    series = series - term;
    count--;
    fraction = (target-series)/term;
    series = series +fraction*term;
    accSeries += fraction*series;
  }
  return (count+fraction)/frameRate;
}

// build the list of soldiers who may be suppressed by this round
class BuildPersonsList
{
  ManCargo &_persons;
  ShotShell *_shot;
  AutoArray<int> &_times;

public:
  BuildPersonsList(ManCargo &array,ShotShell *shot,AutoArray<int> &times):_persons(array),_shot(shot),_times(times){}
  __forceinline bool operator () (Entity *veh) const
  {
    // Don't do all the calculations if suppression is turned off.
    // Will save some cycles doing it here rather than at the end.
    if (!Glob.config.IsEnabled(DTSuppressPlayer))
      return false;

    Person *person = dyn_cast<Person>(veh);
    if (!person) return false;

      Man *man = dyn_cast<Man>(person);
    if (!man) return false;
  
    if (!_shot) return false;

    EntityAI *owner = _shot->GetOwner();
    if (!owner) return false;

    TargetSide   shootersSide = owner->GetTargetSide();
    Vector3   shooterPos = owner->Position();
      Vector3     targetPos = man->Position();
      float  dist = targetPos.Distance2(shooterPos);

      // don't suppress self
    if (man == owner) 
        return false;
    
      // a friendly close to the firer - can't be suppressed
    if (shootersSide==man->GetTargetSide() && dist<FRIENDLY_NO_SUPPRESSION_RADIUS2) 
        return false;

      // dead
    if (man->IsDamageDestroyed())
        return false;

        Vector3 roundPosition = _shot->Position();
        _shot->SetStartingLocation(roundPosition);
        float roundX = roundPosition.X();
        float roundY = roundPosition.Y();
        float roundZ = roundPosition.Z();
        Vector3 roundSpeed = _shot->Speed();
        float speedX = roundSpeed.X();
        float speedY = roundSpeed.Y();
        float speedZ = roundSpeed.Z();
        Vector3 targetPosition = man->VisiblePosition();
        float targetX = targetPosition.X();
        float targetY = targetPosition.Y();
        float targetZ = targetPosition.Z();
        if (!man->Brain()) return false;
        // Get the appropriate suppression radius for this individual. Will be stored in their morale class.
        // But incase not we'll use a default value as the fallback. Further we then multiply by a tolerance
        // value to not exclude by accident any in this early stage.
        float suppressionRadius = SUPPRESSION_RADIUS_INTEREST;
        Morale *morale = man->GetMorale();
        if (morale)
          suppressionRadius = morale->GetSuppressionRadius();
        suppressionRadius *= SUPPRESSION_TOLERANCE_CHECK1;
        
        // now solve quadratic equation of the form a.t^2 + b.t + c = 0
        float a = speedX*speedX+speedZ*speedZ;
        float b = 2.0*(speedX*roundX-targetX*speedX + speedZ*roundZ-speedZ*targetZ);
        float c = targetX*targetX  + targetZ*targetZ + roundX*roundX + roundZ*roundZ - 2.0*targetX*roundX - 2.0*targetZ*roundZ - suppressionRadius*suppressionRadius;
        float det = b*b - 4.0*a*c;
        
        // if determinant >=0 then there is a solution and hence the line intersects the circle
        if (det>=0) 
        {
          float time1 = (-b+sqrt(det))/(2.0*a);
          float time2 = (-b-sqrt(det))/(2.0*a);

          // solutions may be in negative time [invalid], and there may also only be 1 solution (slips circle). hence
          // ensure the closest valid solution ends up in time1
          if (time1>0 || time2>0) 
          {
            if (time1<0) time1=0;
            if (time2<0) time2=0;
            if (time1>time2) 
            {
              float tmp = time1;
              time1 = time2;
              time2 = tmp;
            }

            // so, circle gets touched. now we need to do some height prediction and checking to see that
            // it will pass at the right height and thus be inside the sphere. because of friction the
            // exact solution for this is "intensive" [and we'd need more accurate time1/time2 values that
            // also took friction into account]. hence we'll just allow a greater margin for error in the
            // vertical dimension
            float predictedY1 = roundY + speedY*time1 - 4.9*time1*time1;
            float predictedY2 = roundY + speedY*time2 - 4.9*time2*time2;
            if (fabs(predictedY1-targetY)<suppressionRadius*SUPPRESSION_VERTICAL_TOLERANCE ||
                fabs(predictedY2-targetY)<suppressionRadius*SUPPRESSION_VERTICAL_TOLERANCE) 
            {
              // now get a very accurate (incorporating friction aspect) estimation of the time that
              // the round will reach the edge of the circle
              float horizDistance = sqrt((roundX-targetX)*(roundX-targetX) + (roundZ-targetZ)*(roundZ-targetZ));
              float horizSpeed = sqrt(speedX*speedX+speedZ*speedZ);
              float accurateTime = SolveExpansion(ESTIMATED_FRAMES_PER_SEC*horizDistance/horizSpeed,ESTIMATED_FRICTION,ESTIMATED_FRAMES_PER_SEC);
              int n = _persons.Size();
              _persons.Resize(n+1);
              _persons.Set(n).man = person;
              n = _times.Size();
              _times.Resize(n+1);

              // time in the future in milliseconds that round will suppress (add tiny bit more to ensure will pass into circle)
              _times[n] = Glob.time.toInt()+(int)(accurateTime*1000)+ADDITIONAL_SUPPRESS_MILLISECONDS;   
            }   // height check
          } // at least one of the times positive
        }  // det > 0 

    return false;
  }
};

void ShotShell::SetStartingLocation(Vector3 pos) 
{ 
  _startingLocation = pos; 
}

// shell method called from vehicleAI which invokes the real worker above
void ShotShell::BuildSuppressionList() 
{
  BuildPersonsList build(_potentialSuppressed,this,_timesToSuppress);
  GWorld->ForEachVehicle(build);
}

void ShotShell::CheckForSuppression() 
{
  Vector3 roundPosition = Position();
  int now = Glob.time.toInt();
  for (int i=0;i<_timesToSuppress.Size();i++) 
  {
    // has round survived long enough to fly past/near the soldier that might be suppressed?
    if (now>_timesToSuppress[i]) 
    {   
      Person *person = _potentialSuppressed[i];
      if (person && person->Brain())
      {
        // Get the appropriate suppression radius for this individual. Will be stored in their morale class.
        // But incase not we'll use a default value as the fallback.
        float suppressionRadius = SUPPRESSION_RADIUS_INTEREST;
        Man *man = dyn_cast<Man>(person);
        if (man) 
        {
          Morale *morale = man->GetMorale();
          if (morale) suppressionRadius = morale->GetSuppressionRadius();
        }

        //        float skill = person->Brain()->GetRawAbility();
        // skill of soldier impacts radius of suppression. Most skilled have half the radius. no skill have full.
        //        float suppressionRadius = SUPPRESSION_RADIUS_INTEREST*(1-skill/2.0);
        
        // final check. we need to see if the round really came close enough to cause suppression. Likely the round
        // has jumped past the target. hence do some simple 3D geometry. calculate equation of line from location fired
        // from to current location. then find the point/distance where the target is closest to that line. if its
        // below the suppression threshold then suppress.
        float startX = _startingLocation.X();
        float startY = _startingLocation.Y();
        float startZ = _startingLocation.Z();
        Vector3 currentLoc = Position();
        float finalX = currentLoc.X();
        float finalY = currentLoc.Y();
        float finalZ = currentLoc.Z();
        Vector3 targetLoc = person->VisiblePosition();
        float targetX = targetLoc.X();
        float targetY = targetLoc.Y();
        float targetZ = targetLoc.Z();
        float a = (finalX-startX)*(finalX-startX) + (finalY-startY)*(finalY-startY) + (finalZ-startZ)*(finalZ-startZ);
        float b = (startX-targetX)*(finalX-startX) + (startY-targetY)*(finalY-startY) + (startZ-targetZ)*(finalZ-startZ);
        
        // what fraction along the line from startingLocation to currentLoc was closest to target
        float closestFraction;
        if (a!=0)
          closestFraction = -b/a;
        else
          closestFraction = 0;
        if (closestFraction<0)
          closestFraction = 0;
        else if (closestFraction>1)
          closestFraction = 1;
        float closestX = startX + closestFraction*(finalX-startX);
        float closestY = startY + closestFraction*(finalY-startY);
        float closestZ = startZ + closestFraction*(finalZ-startZ);
        float closestDistance = sqrt((closestX-targetX)*(closestX-targetX) + (closestY-targetY)*(closestY-targetY) + (closestZ-targetZ)*(closestZ-targetZ));

        if (closestDistance<=suppressionRadius) 
        {
          // this unit is suppressed
          person->Suppress(this,closestDistance,Vector3(closestX,closestY,closestZ));
          //GWorld->GetRealPlayer()->Suppress(this,closestDistance,Vector3(closestX,closestY,closestZ));
          // Suppression Event handler goes in here!
          const AmmoType *ammo = this->Type();
          RString name = ammo ? ammo->GetName() : RString();
          EntityAI *eai = dyn_cast<EntityAI>(person);
          if (eai) eai->OnEvent(EESuppressed,this->GetOwner(),name,closestDistance,Vector3(closestX,closestY,closestZ));
        }        
      }
      _potentialSuppressed.Delete(i);
      _timesToSuppress.Delete(i);
      i--;
    } // if
  } // for
}
#endif

/**
@param target
@param real shot which is not real is visual effect only
*/
Shot *NewShot(EntityAI *parent, const AmmoType *type, const WeaponModePtr *ptr, Object *target, bool real, bool showTracer)
{
  Shot *v = NULL;
  type->VehicleAddRef();
  switch(type->_simulation)
  {
    case AmmoShotShell:
      v = new ShotShell(parent,type,real);
      break;
    case AmmoShotBullet:
      v = new ShotBullet(parent,type,real,showTracer);
      break;
    case AmmoShotSpread:
      v = new ShotSpread(parent,type, ptr, real);
      break;
    case AmmoShotMissile:
      Assert(real);
      v = new Missile(parent,type,target);
      break;
    case AmmoShotRocket:
      Assert(real);
      v = new Missile(parent,type,target);
      break;
    case AmmoShotIlluminating:
      Assert(real);
      v = new Flare(parent,type);
      break;
    case AmmoShotSmoke:
      Assert(real);
      v = new SmokeShell(parent,type);
      break;
    case AmmoShotTimeBomb:
      Fail("Time bomb no longer supported");
    // fall through
    case AmmoShotPipeBomb:
      Assert(real);
      v = new PipeBomb(parent,type);
      break;
    case AmmoShotMine:
      Assert(real);
      v = new Mine(parent,type);
      break;
    default:
      ErrF("Unsupported ammo type (type name %s)",(const char *)type->GetName());
    //return NULL;
  }
  type->VehicleRelease();
  if (v)
    v->SetID(GLandscape->NewObjectID());
  return v;
}

#if SHOT_DEBUGGING
#pragma optimize("", on)
#endif
