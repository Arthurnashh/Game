// weapons table

#include "wpch.hpp"

#include "landscape.hpp"
#include "weapons.hpp"
#include "global.hpp"
#include "vehicleAI.hpp"
#include "AI/ai.hpp"
#include "world.hpp"
#include "fileLocator.hpp"
#include "rtAnimation.hpp"
#include "Network/network.hpp"
#include <Es/Algorithms/qsort.hpp>
#include <Es/ErrorProp/errorProp.hpp>

void RandomSound::Load( ParamEntryPar entry, const char *name )
{
  ParamEntryVal list=entry>>name;
  _pars.Realloc(list.GetSize()/2);
  _pars.Resize(0);
  for( int i=0; i<list.GetSize(); i+=2 )
  {
    RString singleName=list[i];
    // load single sound parameter
    SoundProbab extPars;
    ParamEntryVal single=entry>>singleName;
    GetValue(extPars, single);
    extPars._probability=list[i+1];
    _pars.Add(extPars);
  }
  _pars.Compact();
}

void PPEffectType::Load(ParamEntryPar cfg)
{
  // effect type
  _effectType = cfg >> "type";
  _priority = cfg >> "priority";

  // load effect parameters
  ParamEntryVal parEntry = cfg >> "params";

  if (parEntry.IsArray())
  {
    // number of pars
    int size = parEntry.GetSize();
    _pars.Realloc(size);
    _pars.Resize(size);

    // load effect parameters
    for (int i = 0; i < size; i++)
    {
      _pars[i] = parEntry[i];
    }
  }
}

#if _HELISIM_RENDER_TO_TEXTURE

void R2TCameraDescription::Load(ParamEntryPar cfg)
{
  SetToDefault();

  ParamEntryPtr pointEntry = cfg.FindEntry("pointPosition");
  if (pointEntry)
  {
    _posName = *pointEntry;
    _pos = VZero; // by default
  }
  else
    GetValue(_pos, cfg >> "position");

  ParamEntryPtr pointDirEntry = cfg.FindEntry("pointDirection");
  if (pointDirEntry)
  {
    _dirName = *pointDirEntry;
    _dir = VForward; // by default
  }
  else
  {
    GetValue(_dir, cfg >> "direction");
    _dir.Normalize();
  }

  if (cfg.FindEntry("fov")) _fov = cfg >> "fov";

  if (cfg.FindEntry("renderQuality"))
  {
    int rQ = cfg >> "renderQuality";
    saturate(rQ, QLow, QHigh);
    _renderQuality = (R2TRenderQuality) rQ;
  }

  if (cfg.FindEntry("renderVisionMode"))
  {
    int vM = cfg >> "renderVisionMode";
    saturate(vM, PENormal, PEMirror);
    _postEffect = (R2TCameraPostEffect) vM;
  }

  ParamEntryPtr effectParsEntry = cfg.FindEntry("pars");
  if (effectParsEntry && effectParsEntry->IsArray())
  {
    bool error = false;
    for (int i = 0; i < effectParsEntry->GetEntryCount(); i++)
    {
      if (error)
        break;

      ParamEntryVal val = effectParsEntry->GetEntry(i);

      if (val.IsFloatValue())
      {
        _pePars.Append(val);
      }
      else if (val.IsArray()) 
      {
        for (int j = 0; j < val.GetSize(); j++)
        {
          if (val[j].IsFloatValue())
            _pePars.Append(val[j]);
          else
            error = true;
        }
      }
      else
        error = true;
    }

    if (error)
    {
      RptF("Render target vision mode pars loading failed.");
      _pePars.Resize(0);
    }
    else
    {
      _peParsUpdated = true;
      _pePars.Compact();
    }
  }
}

R2TCameraDescription::R2TCameraDescription()
{
  SetToDefault();
}

void R2TCameraDescription::SetToDefault()
{
  _pos = VZero;
  _dir = VForward;
  _fov = 0.7f;
  _posIndex = -1;
  _dirIndex = -1;
  _turretTypeIndex = -1;
  _postEffect = PENormal;
  _renderQuality = QLow;
   _peParsUpdated = false;
}

LSError R2TCameraDescription::Serialize(ParamArchive &ar)
{
  CHECK(::Serialize(ar, "pos", _pos, 1, VZero));
  CHECK(::Serialize(ar, "dir", _dir, 1, VForward));
  CHECK(ar.Serialize("posName", _posName, 1));
  CHECK(ar.Serialize("dirName", _dirName, 1));
  CHECK(ar.Serialize("posIndex", _posIndex, 1, -1));
  CHECK(ar.Serialize("dirIndex", _dirIndex, 1, -1));
  CHECK(ar.Serialize("turretTypeIndex", _turretTypeIndex, 1, -1));
  CHECK(ar.Serialize("fov", _fov, 1, 0.7f));
  CHECK(ar.SerializeArray("pars", _pePars, 1));
  
  int saveEnum = _postEffect;
  CHECK(ar.Serialize("pEffect", saveEnum, 1 , PENormal));
  if (ar.IsLoading()) _postEffect = (R2TCameraPostEffect) saveEnum;
  saveEnum = _renderQuality;
  CHECK(ar.Serialize("pEffect", saveEnum, 1 , QLow));
  if (ar.IsLoading()) 
  {
    _peParsUpdated = true;
    _renderQuality = (R2TRenderQuality) saveEnum;
  }

  return LSOK;
}

bool R2TTgtDescription::Load(ParamEntryPar cfg)
{
  if (!cfg.FindEntry("renderTarget")) return false;
 
  _name = cfg >> "renderTarget";

  int entries = cfg.GetEntryCount();
  if (entries < 2) return false;

  for (int i = 0; i < entries; i++)
  {
    ParamEntryVal entry = cfg.GetEntry(i);

    if (!entry.IsClass()) continue;

    R2TCameraDescription &camDesc = _camDesc.Append();
    camDesc.Load(entry);
  }

  _camDesc.Compact();
  return true;
}

LSError R2TTgtDescription::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("name", _name, 1));
  CHECK(ar.Serialize("camDesc", _camDesc, 1));

  return LSOK;
}

const R2TCameraDescription& R2TTgtPars::Next()
 { 
   DoAssert(_tgtDesc._camDesc.Size() > 0);

   if (_currentIndex >= _tgtDesc._camDesc.Size())
   {
     _currentIndex = 0;
   }

   return _tgtDesc._camDesc[_currentIndex++]; 
 }
#endif


const SoundPars &RandomSound::SelectSound( float probab ) const
{
  Assert( _pars.Size()>0 );
  if( _pars.Size()<=0 )
  {
    static const SoundPars nil;
    return nil;
  }
  int i;
  for( i=0; i<_pars.Size()-1; i++ )
  {
    probab-=_pars[i]._probability;
    if( probab<=0 ) return _pars[i];
  }
  return _pars[i];
}

bool RandomSound::PreloadSounds() const
{
  for (int i = 0; i < _pars.Size(); i++)
  {
    if (_pars[i].name.GetLength() > 0)
    {
      Ref<AbstractWave> wave = GSoundScene->Open(_pars[i].name);

      if (wave)
      {
        if (!wave->PreparePlaying()) return false;
      }
    }
  }

  return true;
}

// =================================================
// CameraShakePars implementation
// =================================================
void CameraShakePars::Load(ParamEntryPar cfg)
{
  if (!cfg.IsClass())  
    return; //bad cfg

  ConstParamEntryPtr par = cfg.FindEntry("power");
  if (par)
  {
    _power = *par;
  }
  par = cfg.FindEntry("duration");
  if (par)
  {
    _duration = *par;
  }
  par = cfg.FindEntry("frequency");
  if (par)
  {
    _frequency = *par;
  }
  par = cfg.FindEntry("distance");
  if (par)
  {
    _distance = *par;
  }
  par = cfg.FindEntry("minSpeed");
  if (par)
  {
    _minSpeed = *par;
  }

  _isValid = true;
}

#if _VBS3 //MuzzleEvent

DEFINE_ENUM(AmmoEvent,AE,AMMO_EVENT_ENUM)

void AmmoType::OnEvent(AmmoEvent event, const GameValue &pars) const
{
  // execute type based event handler
  RString handler = _eventHandlers[event];
  GameVarSpace local(false);
  GameState *gstate = GWorld->GetGameState();
  int gshandle = gstate->BeginContext(&local);
  gstate->VarSetLocal("_this",pars,true);
  gstate->Execute(handler, GameState::EvalContext::_default, GWorld->GetMissionNamespace());
  gstate->EndContext(gshandle);
}

bool AmmoType::IsEventHandler(AmmoEvent event) const
{
  RString handler = _eventHandlers[event];
  if (handler.GetLength()>0) return true;
  return false;
}
#endif

void AmmoType::InitShape()
{
  //LogF("Init shape ammo %s",(const char *)GetName());
  base::InitShape();
  // load cartridge shape
  RStringB cartridge = ParClass()>>"cartridge";
  if (cartridge.GetLength()>0)
  {
    Ref<EntityType> type=VehicleTypes.New(cartridge);
    EntityAIType *typeAI = dynamic_cast<EntityAIType *>(type.GetRef());
    if (typeAI)
    {
      _cartridgeType = typeAI;
      _cartridgeType->VehicleAddRef(); // force loading shape
    }
    else
    {
      RptF("Warning: unable to create ammo cartridge: %s",(const char *)cartridge);
  }
  }
  RStringB proxyShape = ParClass()>>"proxyShape";
  if (proxyShape.GetLength()>0)
  {
    LODShape *pshape = Shapes.New(::GetShapeName(proxyShape), false, false);
    _proxyShape = pshape;
  }

  RStringB decalShape = ParClass()>>"decalModel";
  if (decalShape.GetLength()>0)
  {
    LODShape *dshape = Shapes.New(::GetShapeName(decalShape), _shapeReversed, _shadowEnabled);
    _decalShape = dshape;
  }
}

void AmmoType::DeinitShape()
{
  //LogF("Deinit shape ammo %s",(const char *)GetName());
  if (_cartridgeType)
  {
    _cartridgeType->VehicleRelease(); // allow releasing shape
    _cartridgeType.Free();
  }
  _proxyShape.Free();
  _decalShape.Free();
  base::DeinitShape();
}

void AmmoType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);
  //LogF("Load ammo %s",(const char *)GetName());

  #define GET_PAR(x) x=ParClass(#x)
  GET_PAR(cost);
  GET_PAR(hit);
  GET_PAR(indirectHit);
  GET_PAR(indirectHitRange);
  GET_PAR(maxControlRange);
  GET_PAR(maneuvrability);
  GET_PAR(trackOversteer);
  GET_PAR(trackLead);
  GET_PAR(thrust);
  GET_PAR(thrustTime);
  GET_PAR(initTime);
  GET_PAR(explosionTime);
  GET_PAR(fuseDistance);
  GET_PAR(maxSpeed);
  GET_PAR(sideAirFriction);
  GET_PAR(simulationStep);
  
  _airFriction = _par->ReadValue("airFriction",-0.0005f);
  _coefGravity = _par->ReadValue("coefGravity",1.0f);
  _typicalSpeed2 = Square((*_par)>>"typicalSpeed");

  _tracerScale      = _par->ReadValue("tracerScale",1.0f);
  _nvgOnly          = _par->ReadValue("nvgOnly",false);
  _tracerStartTime  = _par->ReadValue("tracerStartTime",-1.0f); //disables tracer
  _tracerEndTime    = _par->ReadValue("tracerEndTime",4.0f);
  _affectSkeleton   = _par->ReadValue("affectSkeleton", 0.0f);

  GET_PAR(visibleFire);
  GET_PAR(audibleFire);
  GET_PAR(visibleFireTime);

#if _VBS3
  audibleFireTime = _par->ReadValue("audibleFireTime", visibleFireTime);
#endif

  GET_PAR(irLock);
  GET_PAR(laserLock); // laser lock only
  GET_PAR(nvLock);    // nv marker

  airLock = (AmmoTargetType)(par>>"airLock").GetInt();

  GET_PAR(manualControl);
  GET_PAR(explosive);
  GET_PAR(caliber);
  GET_PAR(deflecting);

  _deflectionSlowDown = _par->ReadValue("deflectionSlowDown",0.8f);

  maverickweaponIndexOffset  = _par->ReadValue<int>("maverickweaponIndexOffset",0);

  #undef GET_PAR

  _explosionEffects = ParClass("explosionEffects");
  _craterEffects = ParClass("craterEffects");
  ParamEntryVal cls = (*_par) >> "HitEffects";
  int n = cls.GetEntryCount();
  _hitEffects.Realloc(n);
  _hitEffects.Resize(n);
  for (int i=0; i<n; i++)
  {
    ParamEntryVal entry = cls.GetEntry(i);
    _hitEffects[i]._name = entry.GetName();
    _hitEffects[i]._effects = entry;
  }
  _effectsMissile = ParClass("effectsMissile");
  _effectsSmoke = ParClass("effectsSmoke");
  _decalClass = ParClass("decalClass");

  if (deflecting>0) deflecting = sin(HDegree(deflecting));
  _timeToLive = ParClass("timeToLive");
  if (_timeToLive<0) _timeToLive = FLT_MAX;
  if (explosionTime<=0) explosionTime = FLT_MAX;
  
  ConstParamEntryPtr entry = par.FindEntry("defaultMagazine");
  if (entry) _defaultMagazine = *entry;
  
  //LogF("%s: man %.3f",name,maneuvrability);

  // ammo is by default reversed
  _shapeReversed = true;

  /*
  RString model=ParClass("model");
  if( model.GetLength()>0 )
  {
    _shape=Shapes.New(::GetShapeName(model),true,IS_SHADOW_VEHICLE);
  }
  else _shape=NULL;
  */
  _texture=NULL;
  // hit sounds for different materials
  _hit[SurfaceInfo::SHGround].Load(ParClass(),"hitGroundSoft"); // TODO: replace with a factory
  _hit[SurfaceInfo::SHGroundHard].Load(ParClass(),"hitGroundHard");
  _hit[SurfaceInfo::SHMan].Load(ParClass(),"hitMan");
  _hit[SurfaceInfo::SHArmor].Load(ParClass(),"hitArmor");
  _hit[SurfaceInfo::SHIron].Load(ParClass(),"hitIron");
  _hit[SurfaceInfo::SHBuilding].Load(ParClass(),"hitBuilding");
  _hit[SurfaceInfo::SHFoliage].Load(ParClass(),"hitFoliage");
  _hit[SurfaceInfo::SHWood].Load(ParClass(),"hitWood");
  _hit[SurfaceInfo::SHGlass].Load(ParClass(), "hitGlass");
  _hit[SurfaceInfo::SHGlassArmored].Load(ParClass(), "hitGlassArmored");
  _hit[SurfaceInfo::SHPlastic].Load(ParClass(), "hitPlastic");
  _hit[SurfaceInfo::SHConcrete].Load(ParClass(), "hitConcrete");
  _hit[SurfaceInfo::SHRubber].Load(ParClass(), "hitRubber");
  _hit[SurfaceInfo::SHMetalPlate].Load(ParClass(), "hitMetalplate");
  _hit[SurfaceInfo::SHMetal].Load(ParClass(), "hitMetal");
  _hit[SurfaceInfo::SHDefault].Load(ParClass(), "hitDefault"); // used when no suitable material is found
  
  ConstParamEntryPtr bulletEntry = par.FindEntry("bulletFly");
  if (bulletEntry)
  {
    _bulletFly.Load(ParClass(), "bulletFly");
  }

  GetValue(_soundFly, ParClass("soundFly"));
  GetValue(_soundEngine, ParClass("soundEngine"));
  GetValue(_supersonicCrackNear, ParClass("supersonicCrackNear"));
  GetValue(_supersonicCrackFar, ParClass("supersonicCrackFar"));

  _tracerColor = GetPackedColor(ParClass("tracerColor"));
  _tracerColorR = GetPackedColor(ParClass("tracerColorR"));

  RString simName=ParClass("simulation");
  _simulation=AmmoNone;
  if( !strcmpi(simName,"shotShell") ) _simulation=AmmoShotShell;
  else if( !strcmpi(simName,"shotMissile") ) _simulation = AmmoShotMissile;
#if 0 && _VBS3 
  //TODO: first check all places where AmmoShotMissile is used
  else if( !strcmpi(simName,"shotRocket") ) _simulation = AmmoShotRocket;
#else
  else if( !strcmpi(simName,"shotRocket") ) _simulation = AmmoShotMissile;
#endif
  else if( !strcmpi(simName,"shotBullet") ) _simulation = AmmoShotBullet;
  else if( !strcmpi(simName,"shotSpread") ) _simulation = AmmoShotSpread;  
  else if( !strcmpi(simName,"shotIlluminating") ) _simulation=AmmoShotIlluminating;
  else if( !strcmpi(simName,"shotSmoke") ) _simulation = AmmoShotSmoke;
  else if( !strcmpi(simName,"shotTimeBomb") ) _simulation = AmmoShotTimeBomb;
  else if( !strcmpi(simName,"shotPipeBomb") ) _simulation = AmmoShotPipeBomb;
  else if( !strcmpi(simName,"shotMine") ) _simulation = AmmoShotMine;
  else if( !strcmpi(simName,"shotStroke") ) _simulation = AmmoShotStroke;
  else if( !strcmpi(simName,"") ) _simulation = AmmoNone;
  else
  {
    ErrF("%s: Bad ammo simulation %s",(const char *)GetName(),(const char *)simName);
  }

#if _VBS3 // AmmoEvent
  ConstParamEntryPtr array = par.FindEntry("EventHandlers");
  if (array)
  {
    for (int i=0; i<NAmmoEvent; i++)
    {
      AmmoEvent e = (AmmoEvent)i;
      RString name = FindEnumName(e);
      ConstParamEntryPtr entry = array->FindEntry(name);
      if (entry)
      {
        RStringB expression = *entry;
        _eventHandlers[e] = expression;
      }
    }
  }
#endif

  _whistleDist = ParClass("whistleDist");
  _whistleOnFire = ParClass("whistleOnFire");

  // read camera shake params
  ParamEntryPtr camShakePar = par.FindEntry("CamShakeExplode");
  if (camShakePar)
  {
    _camShakeExplode.Load(*camShakePar);
  }
  camShakePar = par.FindEntry("CamShakeFire");
  if (camShakePar)
  {
    _camShakeFire.Load(*camShakePar);
  }
  camShakePar = par.FindEntry("CamShakePlayerFire");
  if (camShakePar)
  {
    _camShakePlayerFire.Load(*camShakePar);
  }
  camShakePar = par.FindEntry("CamShakeHit");
  if (camShakePar)
  {
    _camShakeHit.Load(*camShakePar);
  }
}

AmmoType::AmmoType( ParamEntryPar name )
:base(name), _deflectionSlowDown(0.8f)
{
  airLock = AmmoLockAirGroundTT;
}

RString AmmoType::GetHitEffects(RString name) const
{
  for (int i=0; i<_hitEffects.Size(); i++)
  {
    if (stricmp(_hitEffects[i]._name, name) == 0)
      return _hitEffects[i]._effects;
  }
  return "ImpactEffectsSmall";
}

// class WeaponModeType

WeaponModeType::WeaponModeType()
{
}

void WeaponModeType::Init(ParamEntryPar cls)
{
  _parClass = cls.GetClassInterface();

  _displayName = cls >> "displayName";
  _mult = cls >> "multiplier";
  _burst = cls >> "burst"; // how many bullets are there in single burst

  //GetValue(_sound, cls >> "sound");
  
  _beginSound.Load(cls, "soundBegin"); // semi: single shot variants, burst: first shot, full auto: first shot
  _soundDuration = 0;
  
  _reloadTime = cls >> "reloadTime";
  _dispersion = cls >> "dispersion";
  _recoilName = cls >> "recoil";
  _recoilProneName = cls >> "recoilProne";
  if (_recoilProneName.GetLength()==0) _recoilProneName=_recoilName;
  _autoFire = cls >> "autoFire";
  _aiRateOfFire = cls >> "aiRateOfFire";
  _aiRateOfFireDistance = cls >> "aiRateOfFireDistance";
  _soundContinuous = cls>>"soundContinuous";
  _soundBurst = cls>>"soundBurst";
    /// when burst is fired, only a first bullet is making a sound


  _useAction = cls >> "useAction";
  _useActionTitle = cls >> "useActionTitle";

  _showToPlayer = cls >> "showToPlayer";

  #define GET_PAR(x) x = cls >> #x;
  GET_PAR(minRange); GET_PAR(minRangeProbab);
  GET_PAR(midRange); GET_PAR(midRangeProbab);
  GET_PAR(maxRange); GET_PAR(maxRangeProbab);
  GET_PAR(artilleryDispersion);
  GET_PAR(artilleryCharge);
  #undef GET_PAR

  invMidRangeMinusMinRange=midRange>minRange ? 1/(midRange-minRange) : 1e10;
  invMidRangeMinusMaxRange=maxRange>midRange ? 1/(midRange-maxRange) : -1e10;

  // premultiply with probabilities
  invMidRangeMinusMinRange*=(midRangeProbab-minRangeProbab);
  invMidRangeMinusMaxRange*=(midRangeProbab-maxRangeProbab);
}

void ViewPars::Load(ParamEntryPar cfg)
{
#define GET_PAR(x) _##x=cfg>>#x
  GET_PAR(initAngleY);GET_PAR(minAngleY);GET_PAR(maxAngleY);
  GET_PAR(initAngleX);GET_PAR(minAngleX);GET_PAR(maxAngleX);
  GET_PAR(initFov);GET_PAR(minFov);GET_PAR(maxFov);
#undef GET_PAR

#define _ZERO_MINMAX_MOVE_DEFAULTS 1
#if _PUBLICBETA
 #define GET_PAR(x,def) if(cfg.FindEntry(#x)) {_##x=cfg>>#x;}else{_##x=def;}
#elif _ZERO_MINMAX_MOVE_DEFAULTS
 // zero defaults required when Older project (ArmA 2 OA, ArmA 2) content should be used as mod inside new project (such as TKOH), see news:jl1fa0$pos$1@new-server.localdomain
 #define GET_PAR(x,def) if(cfg.FindEntry(#x)) {_##x=cfg>>#x;}else{_##x=0.0f;}
#else
 #define GET_PAR(x,def) _##x=cfg>>#x
#endif

  GET_PAR(minMoveX,-0.1);GET_PAR(maxMoveX,0.1);GET_PAR(minMoveY,-0.5);GET_PAR(maxMoveY,0.03);GET_PAR(minMoveZ,-0.1);GET_PAR(maxMoveZ,0.1);
  if (_minMoveX>0) _minMoveX=0;
  if (_minMoveY>0) _minMoveY=0;
  if (_minMoveZ>0) _minMoveZ=0;
  if (_maxMoveX<0) _maxMoveX=0;
  if (_maxMoveY<0) _maxMoveY=0;
  if (_maxMoveZ<0) _maxMoveZ=0;
#undef GET_PAR
  saturateMax(_maxFov,_initFov); // force: fov is always in valid range
  saturateMin(_minFov,_initFov);

  saturateMax(_minFov, 0.0001);
  saturateMax(_maxFov, 0.0001);

  if (cfg.FindEntry("directionStabilized")) _directionStabilized = cfg >> "directionStabilized";
  else _directionStabilized= false;

  if (cfg.FindEntry("continuous")) _continuous = cfg >> "continuous";
  else _continuous = 0; //values >1 means continuous zoom is to be used

  if (cfg.FindEntry("opticsDisplayName")) _opticsDisplayName = cfg >> "opticsDisplayName";

#if 1
  if (cfg.FindEntry("visionMode"))
  {
    ParamEntryVal entry = cfg >> "visionMode";
    bool tiWanted = false;

    if (entry.IsArray())
    {
      int count = entry.GetSize();
      _visionModes.Realloc(count);
      _visionModes.Resize(count);

      for (int i = 0; i < count; i++)
      {
        const IParamArrayValue &arVal = entry[i];

        RString effectName = arVal;
        VisionMode vMode = GetEnumValue<VisionMode>(cc_cast(effectName));

        if (vMode < 0 || vMode >= VMCount) vMode = VMNormal;

        _visionModes[i] = vMode;
        if (vMode == VMTi) tiWanted = true;
      }

      if (tiWanted)
      {
        if (cfg.FindEntry("thermalMode"))
        {
          entry = cfg >> "thermalMode";

          int count = entry.GetSize();
          _thermalModes.Realloc(count);
          _thermalModes.Resize(count);

          for (int i = 0; i < count; i++)
          {
            const IParamArrayValue &arVal = entry[i];
            _thermalModes[i] = arVal;
          }
        }
        else
        {
          RptF(Format("Warning: Thermal vision mode defined, but thermal modes not. Optics: %s",
            _opticsDisplayName.GetLength() ? cc_cast(_opticsDisplayName) : ""));
        }
      }
    }
  }
#endif //add discrete zoom levels

#if _VBS3
  _initialized = true;
  if(cfg.FindEntry("discretefov"))
  {
    ParamEntryVal discreteFov = cfg >> "discretefov";
    if(discreteFov.GetSize() > 0)
    {
      _discreteFov.Realloc(discreteFov.GetSize());
      _discreteFov.Resize(discreteFov.GetSize());
      for (int i=0; i<discreteFov.GetSize(); i++)
      {
        _discreteFov[i] = discreteFov[i];
      }
    }
  }
  _hasNV = cfg.ReadValue("nightvision", true); //support Arma
  _hasDV = cfg.ReadValue("dayVision",true);

  if(cfg.FindEntry("timodes"))
  {
    ParamEntryVal thermalModes = cfg >> "timodes";
    if(thermalModes.GetSize() > 0)
    {
      _thermalModes.Realloc(thermalModes.GetSize());
      _thermalModes.Resize(thermalModes.GetSize());
      for (int i=0; i<thermalModes.GetSize(); i++)
      {
        _thermalModes[i] = int(thermalModes[i]);
      }
    }
  }
  if(cfg.FindEntry("OpticsModel"))
  {
    _opticsModel = cfg >> "OpticsModel";
    _opticsColor = GetPackedColor(cfg >> "OpticsColor");
  }
  _canSee = cfg.ReadValue("CanSee",0);

  if(!_hasNV && !_hasDV && !_thermalModes.Size())
    Fail(Format("No vision mode supported by optics: %s", cfg.GetName()));
#endif //_VBS3
}

void ViewPars::InitVirtual(CameraType camType, float &heading, float &dive, float &fov) const
{
  fov = _initFov;
  dive = _initAngleX*(H_PI/180);
  heading = _initAngleY*(H_PI/180);
}

#if _VBS3
int ViewPars::GetNextThermalMode(int oldThermalmode) const
{
  if(_thermalModes.Size() == 0)
    return -1;
  ++oldThermalmode;
  if(oldThermalmode >= _thermalModes.Size())
    return -1;
  return oldThermalmode;
}

void ViewPars::DiscretizeValue(float fov, float &prev, float &closest, float &next) const
{
  int n = _discreteFov.Size();
  if(n <= 0)
  {
    prev = closest = next = fov;
    return;
  }

  int closestIndex = n-1;
  for(int i= 0; i < n;++i)
  {
    if(fov < _discreteFov[i])
    {
      if(fov - _discreteFov[i-1] > 0.5 * (_discreteFov[i] - _discreteFov[i-1]))
        closestIndex = i;
      else
        closestIndex = i-1;
      break;
    }
  }
  closest = _discreteFov[closestIndex];
  prev = _discreteFov[closestIndex > 0 ? closestIndex-1 : 0];
  next = _discreteFov[closestIndex + 1 < n ? closestIndex+1 : closestIndex];
}
#endif //_VBS3

void ViewPars::LimitVirtual(CameraType camType, float &heading, float &dive, float &fov) const
{
  float minHeading = _minAngleY*(H_PI/180);
  float maxHeading = _maxAngleY*(H_PI/180);
  if( maxHeading-minHeading<H_PI*15/8 )
  {
    float initHeading = _initAngleY*(H_PI/180);
    minHeading = AngleDifference(minHeading,initHeading);
    maxHeading = AngleDifference(maxHeading,initHeading);
    heading = AngleDifference(heading,initHeading);

    saturate(heading,minHeading,maxHeading);

    heading += initHeading;
  }
  float minDive = _minAngleX*(H_PI/180);
  float maxDive = _maxAngleX*(H_PI/180);
  saturate(dive,minDive,maxDive);
  saturate(fov,_minFov,_maxFov);
}

void VehPosRelatives::Load(ParamEntryPar cfg)
{
#define GET_PAR(x) _##x=cfg>>#x
  GET_PAR(showHeadPhones);
#undef GET_PAR
}

inline void SelectNextVisionMode(VisionMode &mode, const AutoArray<int> &modes)
{
  // find current mode
  int currSelMode = 0;
  for (int i = 0; i < modes.Size(); i++)
  {
    if (modes[i] == mode) 
    {
      currSelMode = i;
      break;
    }
  }

  int nextMode = currSelMode + 1;  
  if (nextMode >= modes.Size()) nextMode = 0;

  Assert(nextMode >= 0 && nextMode < modes.Size());

  mode = (VisionMode)modes[nextMode];
}

inline bool SelectNextThermalMode(int &tiIndex, const AutoArray<int> &modes)
{
  // find current thermal mode
  int currSelMode = 0;
  for (int i = 0; i < modes.Size(); i++)
  {
    if (modes[i] == tiIndex) 
    {
      currSelMode = i;
      break;
    }
  }

  int nextMode = currSelMode + 1;  
  if (nextMode >= modes.Size()) 
  {
    tiIndex = (modes.Size()>0)?modes[0]:-1;
    return false;
  }

  Assert(nextMode >= 0 && nextMode < modes.Size());
  tiIndex = modes[nextMode];

  return true;
}

void ViewPars::NextVisionMode(VisionMode& mode, int &tiModeIndex) const
{
  if (_visionModes.Size() > 0)
  {
    // iterate through thermal modes
    if (!(mode == VMTi && SelectNextThermalMode(tiModeIndex, _thermalModes)))
    {
      // select next vision mode
      SelectNextVisionMode(mode, _visionModes);
    }
  }
}

void ViewPars::GetInitVisionMode(VisionMode& mode, int &tiModeIndex) const
{
  mode = VMNormal;
  tiModeIndex = -1;
  if (_visionModes.Size() > 0) mode = (VisionMode)_visionModes[0];
  if (_thermalModes.Size() > 0) tiModeIndex = _thermalModes[0];
}

void OpticsInfo::NextVisionMode(VisionMode& mode, int &tiModeIndex) const
{
  if (_visionModes.Size() > 0)
  {
    // iterate through thermal modes
    if (!(mode == VMTi && SelectNextThermalMode(tiModeIndex, _thermalModes)))
    {
      // select next vision mode
      SelectNextVisionMode(mode, _visionModes);
    }
  }
}

bool OpticsInfo::HasVisionMode(VisionMode mode) const
{
  for (int i = 0; i < _visionModes.Size(); i++)
  {
    if (_visionModes[i] == mode) { return true; }
  }

  return false;
}

void OpticsInfo::GetInitVisionMode(VisionMode& mode, int &tiModeIndex) const
{
  mode = VMNormal;
  tiModeIndex = -1;
  if (_visionModes.Size() > 0) mode = (VisionMode)_visionModes[0];
  if (_thermalModes.Size() > 0) tiModeIndex = _thermalModes[0];
}

bool FlashLigthInfo::Load(ParamEntryPar cls)
{
  if (cls.GetEntryCount() != 8) return false;

  GetValue(_color, cls >> "color");
  GetValue(_colorAmbient, cls >> "ambient");  
  _positionMem = cls >> "position";
  _directionMem = cls >> "direction";
  _angle = cls >> "angle";
	_innerAngleRatio = cls >> "angleInnerRatio";
  GetValue(_scale, cls >> "scale");
  _radius = cls >> "radius";

  return true;
}

void FlashLigthInfo::Init(const LODShape* shape)
{
  if (shape)
  {
    int mem = shape->FindMemoryLevel();

    _position = shape->NamedPoint(mem, _positionMem);
    Vector3 direction = shape->NamedPoint(mem, _directionMem);
    _direction = _position - direction;
    _direction.Normalize();
  }
}

bool EntityAIType::IsSupply() const
{
  if (GetMaxFuelCargo() > 0) return true;
  if (GetMaxRepairCargo() > 0) return true;
  if (GetMaxAmmoCargo() > 0) return true;
  if (GetItemsCargoWidth() > 0 && GetItemsCargoHeight() > 0) return true;
  if( IsAttendant() ) return true;
  if( IsEngineer() ) return true;
  for (int i=0; i<_weaponCargo.Size(); i++)
  {
    if (_weaponCargo[i].count > 0) return true;
  }
  for (int i=0; i<_magazineCargo.Size(); i++)
  {
    if (_magazineCargo[i].count > 0) return true;
  }
  return _forceSupply;
}

bool EntityAIType::CanStoreWeapons() const
{
  if (GetItemsCargoWidth() > 0 && GetItemsCargoHeight() > 0) return true;
  for (int i=0; i<_weaponCargo.Size(); i++)
  {
    if (_weaponCargo[i].count > 0) return true;
  }
  for (int i=0; i<_magazineCargo.Size(); i++)
  {
    if (_magazineCargo[i].count > 0) return true;
  }

  return false;
}
