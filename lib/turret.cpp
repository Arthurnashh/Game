#include "wpch.hpp"
#include "transport.hpp"
#include "rtAnimation.hpp"
#include "AI/ai.hpp"
#include "AI/abilities.hpp"
#include "Network/network.hpp"
#include <El/Common/randomGen.hpp>
#include "fileLocator.hpp"
#include "keyInput.hpp"


float AnimationSourceTurretBody::GetPhase(const ObjectVisualState &vs, const Animated *obj) const
{
  const Transport *transport = static_cast<const Transport *>(obj);
  const Turret *turret = transport->GetTurret(_path.Data(), _path.Size());
  TurretVisualState const* tvs = (static_cast_checked<TransportVisualState const&>(vs)).GetTurret(_path.Data(), _path.Size());
  return turret ? turret->GetCtrlYRot(*tvs) : 0;
}

float AnimationSourceTurretGun::GetPhase(const ObjectVisualState &vs, const Animated *obj) const
{
  const Transport *transport = static_cast<const Transport *>(obj);
  const Turret *turret = transport->GetTurret(_path.Data(), _path.Size());
  TurretVisualState const* tvs = (static_cast_checked<TransportVisualState const&>(vs)).GetTurret(_path.Data(), _path.Size());
  return turret ? turret->GetCtrlXRot(*tvs) : 0;
}

float AnimationSourceTurretHatch::GetPhase(const ObjectVisualState &vs, const Animated *obj) const
{
  const Transport *transport = static_cast<const Transport *>(obj);
  const Turret *turret = transport->GetTurret(_path.Data(), _path.Size());
  TurretVisualState const* tvs = (static_cast_checked<TransportVisualState const&>(vs)).GetTurret(_path.Data(), _path.Size());
  return turret ? turret->GetCtrlGunnerHatchPos(*tvs) : 0;
}

TurretType::TurretType()
{
  _initElev = 0;
  _initTurn = 0;
  _animGun = -1;
  _animBody = -1;

  _turretHit = -1;
  _gunHit = -1;

  _lodTurnedIn = -1; 
  _lodTurnedOut = -1; 

  _discreteDistanceInitIndex = -1;
  _canEject = false;
}

void TurretType::Load(ParamEntryPar cfg)
{
  _par = cfg.GetClassInterface();

  _minElev=(float)(cfg>>"minElev")*(H_PI/180);
  _maxElev=(float)(cfg>>"maxElev")*(H_PI/180);
  _minTurn=(float)(cfg>>"minTurn")*(H_PI/180);
  _maxTurn=(float)(cfg>>"maxTurn")*(H_PI/180);
  _maxYRotSpeed = (float)(cfg>>"maxHorizontalRotSpeed");
  _maxXRotSpeed = (float)(cfg>>"maxVerticalRotSpeed");
  ConstParamEntryPtr entry = cfg.FindEntry("initElev");
  if (entry) _initElev = (float)(*entry)*(H_PI/180);
  entry = cfg.FindEntry("initTurn");
  if (entry) _initTurn = (float)(*entry)*(H_PI/180);
  //LogF("%s: elev %g..%g",(const char *)cfg.GetContext(),_minElev,_maxElev);
  GetValue(_servoSound, cfg >> "soundServo");

  _animationSourceBody = cfg >> "animationSourceBody";
  _animationSourceGun = cfg >> "animationSourceGun";
  _animationSourceHatch = cfg >> "animationSourceHatch";

  int proxyType = cfg >> "proxyType";
  _proxyType = (CrewPosition)proxyType;
  _proxyIndex = cfg >> "proxyIndex";

  // Lock turret to forward direction when driver is out
  _lockWhenDriverOut = cfg >> "lockWhenDriverOut";
 
  // when speed of vehicle is higher that this value then gunner cannot control movement of his turret, -1 for disable
  _lockWhenVehicleSpeed = cfg >> "lockWhenVehicleSpeed";

  //if true, unit can take control over vehicle
  _isCopilot = cfg >> "isCopilot";

  // Gunner related members
  _gunnerName = cfg >> "gunnerName";

  _gunnerAction = ExtractActionName(cfg >> "gunnerAction");
  _gunnerInAction = ExtractActionName(cfg >> "gunnerInAction");
  _gunnerGetInAction = ExtractActionName(cfg >> "gunnerGetInAction");
  _gunnerGetOutAction = ExtractActionName(cfg >> "gunnerGetOutAction");

  _gunnerOpticsColor = GetPackedColor(cfg >> "gunnerOpticsColor");
  _gunnerOutOpticsColor = GetPackedColor(cfg >> "gunnerOutOpticsColor");

  _canEject = cfg >> "canEject";

  if(cfg.FindEntry("discreteDistance"))
  {
    ParamEntryVal discreteDist = cfg >> "discreteDistance";
    if(discreteDist.GetSize() > 0)
    {
      _discreteDistance.Realloc(discreteDist.GetSize());
      _discreteDistance.Resize(discreteDist.GetSize());
      for (int i=0; i<discreteDist.GetSize(); i++)
        _discreteDistance[i] = discreteDist[i];
    }
    _discreteDistanceInitIndex = cfg >> "discreteDistanceInitIndex";
  }

  if(cfg.FindEntry("OpticsIn"))
  {
    ParamEntryVal entry = cfg >> "OpticsIn";
    for(int i=0; i<entry.GetEntryCount(); i++)
    {
      ViewPars view;
      view.Load(entry.GetEntry(i));
      _viewOptics.Add(view);
    }
  }
  else
  {//to be removed
    ViewPars viewOptics;
    viewOptics.Load(cfg >> "ViewOptics");
    _viewOptics.Add(viewOptics);
  }

  if(cfg.FindEntry("TurretSpec"))
  {
    _turretSpec.Load(cfg >> "TurretSpec");
  }

  if(cfg.FindEntry("OpticsOut"))
  {
    ParamEntryVal entry = cfg >> "OpticsOut";
    for(int i=0; i<entry.GetEntryCount(); i++)
    {
      ViewPars view;
      view.Load(entry.GetEntry(i));
      _viewGunner.Add(view);
    }
  }
  else
  {//to be removed
    ViewPars viewGunner;
    viewGunner.Load(cfg >> "ViewGunner");
    _viewGunner.Add(viewGunner);
  }

#if _VBS3 //ViewOptics classes store more information
  if(cfg.FindEntry("ViewOutOptics"))
    _viewOutOptics.Load(cfg >> "ViewOutOptics");
  else
    _viewOutOptics = _viewOptics;

  if(_viewOptics._opticsModel.GetLength() > 0) //indication that it's a new VBS2 entry
    _gunnerOpticsColor = _viewOptics._opticsColor;
  if(_viewOutOptics._opticsModel.GetLength() > 0) //indication that it's a new VBS2 entry
    _gunnerOutOpticsColor = _viewOutOptics._opticsColor;

# if _VBS3_LASE_LEAD
  _canLase = cfg.ReadValue("canLase",false);
# endif

#endif //_VBS3

  _startEngine = cfg >> "startEngine";

  _outGunnerMayFire = cfg >> "outGunnerMayFire";
  _outGunnerFireAlsoInInternalCamera = cfg >> "gunnerOutFireAlsoInInternalCamera";
  _inGunnerMayFire = cfg >> "inGunnerMayFire";
  _inGunnerFireAlsoInInternalCamera = cfg >> "gunnerFireAlsoInInternalCamera";
  _gunnerForceOptics = cfg >> "gunnerForceOptics";
  _gunnerOpticsShowCursor = cfg >> "gunnerOpticsShowCursor";
  _gunnerOutForceOptics = cfg >> "gunnerOutForceOptics";
  _gunnerOutOpticsShowCursor = cfg >> "gunnerOutOpticsShowCursor";
  _castGunnerShadow = cfg >> "castGunnerShadow";

  _viewGunnerShadow = cfg >> "viewGunnerShadow";
  _viewGunnerShadowDiff = cfg >> "viewGunnerShadowDiff";
  _viewGunnerShadowAmb = cfg >> "viewGunnerShadowAmb";

  _ejectDeadGunner = cfg >> "ejectDeadGunner";
  _hideWeaponsGunner = cfg >> "hideWeaponsGunner";
  _forceHideGunner = cfg >> "forceHideGunner";
  _canHideGunner = cfg >> "canHideGunner";
  _viewGunnerInExternal = cfg >> "viewGunnerInExternal";
  _gunnerUsesPilotView = cfg >> "gunnerUsesPilotView";

  _gunnerCompartments = cfg >> "gunnerCompartments";

  //new parameters to specify which LOD to display
  _lodTurnedIn = cfg.ReadValue("LODTurnedIn", -1);
  _lodTurnedOut = cfg.ReadValue("LODTurnedOut", -1);
#if _VBS3 
  //Limit to gunner, cargo, pilot?
  _overrideTurretName = cfg.ReadValue("canOverride", RString());
  _overrideTurretName.Lower();
#endif

  _stabilizedInAxes = (StabilizedInAxes)(int)(cfg>>"stabilizedInAxes");

  ParamEntryVal cfgOpticsPars = Pars >> "CfgOpticsEffect";

  ParamEntryVal optEntry = cfg >> "gunnerOpticsEffect";
  if (optEntry.IsArray())
  {
    int size = optEntry.GetSize();

    _opticsType.Realloc(size);
    _opticsType.Resize(size);

    for (int i = 0; i < size; i++)
    {
      // effect type
      RString name = optEntry[i];
      if (cfgOpticsPars.FindEntry(name))
      {
        ParamEntryVal eEntry = cfgOpticsPars >> name;
        if (eEntry.IsClass()) _opticsType[i].Load(eEntry);
      }
    }
  }

  ParamEntryVal optOutEntry = cfg >> "gunnerOutOpticsEffect";
  if (optOutEntry.IsArray())
  {
    int size = optOutEntry.GetSize();

    _opticsOutType.Realloc(size);
    _opticsOutType.Resize(size);

    for (int i = 0; i < size; i++)
    {
      RString name = optOutEntry[i];
      if (cfgOpticsPars.FindEntry(name))
      {
        ParamEntryVal eEntry = cfgOpticsPars >> name;
        if (eEntry.IsClass()) _opticsOutType[i].Load(eEntry);
      }
    }
  }

  if(cfg.FindEntry("turretInfoType")) 
  {
    ParamEntryVal types = cfg >> "turretInfoType";
    if(types.IsArray())
    {
      int n = types.GetSize();
      _turretInfoTypes.Realloc(n);
      _turretInfoTypes.Resize(n);
      for (int i=0; i<n; i++) _turretInfoTypes[i] = types[i];
    }

    else
    {
      _turretInfoTypes.Realloc(1);
      _turretInfoTypes.Resize(1);
      _turretInfoTypes[0] = types;
    }
  }

  _turretFollowFreeLook = (cfg >> "turretFollowFreeLook");
}

/*!
\patch 2.01 Date 1/13/2003 by Ondra
- Fixed: AI Aiming with turreted weapons sidewise was offset.
*/

SkeletonIndex TurretType::GetBodyBone(const EntityType *entType, int level) const
{
  if (_animBody<0) return SkeletonIndexNone;
  return entType->GetAnimations()[_animBody]->GetBone(level);
}

SkeletonIndex TurretType::GetGunBone(const EntityType *entType, int level) const
{
  if (_animGun<0) return SkeletonIndexNone;
  return entType->GetAnimations()[_animGun]->GetBone(level);
}

Vector3 TurretType::GetYAxis(const EntityType *entType) const
{
  int memory = entType->GetShape()->FindMemoryLevel();
  if (memory<0) return VZero;
  if (_animBody>=0)
  {
    return entType->GetAnimations()[_animBody]->GetCenter(memory);
  }
  return VZero;
}
Vector3 TurretType::GetXAxis(const EntityType *entType) const
{
  int memory = entType->GetShape()->FindMemoryLevel();
  if (memory<0) return VZero;
  if (_animGun>=0)
  {
    return entType->GetAnimations()[_animGun]->GetCenter(memory);
  }
  return VZero;
}

Vector3 TurretType::GetTurretAimCenter(const EntityType *entType) const
{
  // we need to query the corresponding user animation
  // for this we need to access the animations array - currently in the entity type
  int memory = entType->GetShape()->FindMemoryLevel();
  if (memory<0) return VZero;
  Vector3 ret = VZero;
  if (_animBody>=0)
  {
    ret = entType->GetAnimations()[_animBody]->GetCenter(memory);
    if (_animGun>=0)
    {
      ret[1] = entType->GetAnimations()[_animGun]->GetCenter(memory)[1];
    }
  }
  else if (_animGun>=0)
  {
    ret = entType->GetAnimations()[_animGun]->GetCenter(memory);
  }
  return ret;
}

/*!
\patch 5137 Date 3/1/2007 by Ondra
- Fixed: AI Strykers were not firing when player was not using iron sights / optics.
*/
bool TurretType::InGunnerMayFire( const Person *gunner ) const
{
  CameraType cameraType = GWorld->GetCameraType();
  return (
    // if in gunner may not fire, no need to test anything else
    _inGunnerMayFire && (
      // camera limitations valid only when player is controlling the unit
      gunner!=GWorld->PlayerOn() || !GWorld->PlayerManual() ||
      // player is controlling, check if it can fire using given camera
      cameraType!=CamInternal || _inGunnerFireAlsoInInternalCamera
    )
  );
}

bool TurretType::OutGunnerMayFire( const Person *gunner ) const
{
  CameraType cameraType = GWorld->GetCameraType();
  return (
    // if in gunner may not fire, no need to test anything else
    _outGunnerMayFire && (
      // camera limitations valid only when player is controlling the unit
      gunner!=GWorld->PlayerOn() || !GWorld->PlayerManual() ||
      // player is controlling, check if it can fire using given camera
      cameraType!=CamInternal || _outGunnerFireAlsoInInternalCamera
    )
  );
}

bool TurretType::GunnerMayFire(const TurretContextV &context) const
{
  if (context._turret)
    if (context._turretVisualState->IsGunnerHidden()) 
      return context._turretType->InGunnerMayFire(context._gunner);
    else 
      return context._turretType->OutGunnerMayFire(context._gunner);
  return false;
}

TurretType::TurretType(ParamEntryPar cfg)
:_gunPos(VZero)
{
  Load(cfg);

  // Turrets
  ParamEntryVal turrets = cfg >> "Turrets";
  _turrets.Resize(0);
  for (int i=0; i<turrets.GetEntryCount(); i++)
  {
    ParamEntryVal entry = turrets.GetEntry(i);
    if (!entry.IsClass()) continue;
    _turrets.Add(new TurretType(entry));
  }
  _turrets.Compact();

  // Load weapons info
  _weapons.Load(cfg);

  // Primary turrets
  _primaryGunner = cfg >> "primaryGunner";
  _primaryObserver = cfg >> "primaryObserver";

  // Gunner can get in (otherwise, he can only switch to this position)
  _hasGunner = cfg >> "hasGunner";

  // how much the gunner in this turret command the vehicle
  _commanding = cfg >> "commanding";

  _allowTabLock = true;
  _showAllTargets = showAllNoST;
}

void TurretType::Init(LODShape *shape, EntityAIType *type)
{
  const ParamEntry &par = *_par;

  _weapons.Init(shape, par);

  RStringB bodyName = par >> "body";
  RStringB gunName = par >> "gun";
  _animBody = type->GetAnimations().FindIndex(bodyName);
  _animGun = type->GetAnimations().FindIndex(gunName);
  if (_animBody<0 && bodyName.GetLength()>0)
  {
    RptF("Error: %s: Turret body %s not found while initializing the model %s",cc_cast(par.GetContext()),cc_cast(bodyName),shape->Name());
  }
  if (_animGun<0 && gunName.GetLength()>0)
  {
    RptF("Error: %s: Turret gun %s not found while initializing the model %s",cc_cast(par.GetContext()),cc_cast(gunName),shape->Name());
  }

  // get selection names
  RStringB gunBeg = par >> "gunBeg"; // "usti hlavne"
  RStringB gunEnd = par >> "gunEnd"; // "konec hlavne"

  if (shape->MemoryPointExists(gunBeg))
  {
    Vector3Val beg = shape->MemoryPoint(gunBeg);
    Vector3Val end = shape->MemoryPoint(gunEnd);
    _pos0 = (beg+end)*0.5f;
    _dir0 = (beg-end);
    _dir0.Normalize();
  }
  else
  {
    _pos0 = VZero;
    _dir0 = VForward;
  }

  _gunDir = _dir0;

//////////////////////////////////////////////////////////////////////////
  ParamEntryPtr memoryPointGun = par.FindEntry("memoryPointGun");
  if(memoryPointGun)
  {
    // we have to split single value and array
    if (memoryPointGun->IsArray())
    {
      int n = memoryPointGun->GetSize();
      _gunPosArray.Resize(n);
//      _gunDirArray.Resize(n);
      for (int i=0; i<n; i++)
      {
        RString pointPos = memoryPointGun->operator [](i);
        _gunPosArray[i] = shape ? shape->MemoryPoint(pointPos) : VZero;
      }
//        _gunPosArray.Resize(2);
//        _gunPosArray[0] = Vector3(2,-1.13,2.47);
//        _gunPosArray[1] = Vector3(-2,-1.13,2.47);      
    }
    else
    {
      // single value
      _gunPos= shape ? 
        shape->MemoryPoint(memoryPointGun->operator RString()) :
        VZero;
    }
  }

  if (_gunPosArray.Size()==0 && _gunPos.SquareSize() < 0.1f) _gunPos = _pos0;
//////////////////////////////////////////////////////////////////////////

  RStringB missileBeg = par>>"missileBeg";
  RStringB missileEnd = par>>"missileEnd";
  
  _missilePos = _pos0;
  _missileDir = _dir0;
  
  _missileByTurret = false;
  
  if (shape->MemoryPointExists(missileBeg) && shape->MemoryPointExists(missileEnd))
  {
    Vector3Val beg = shape->MemoryPoint(missileBeg);
    Vector3Val end = shape->MemoryPoint(missileEnd);
    _missilePos = (beg+end)*0.5f;
    _missileDir = (beg-end);
    _missileDir.Normalize();
    _missileByTurret = true;
  }
  
  _neutralXRot = atan2(_dir0.Y(),_dir0.SizeXZ());
  _neutralYRot = 0;

  _gunnerProxy.Init(shape->NLevels());


  if(par.FindEntry("opticsIn"))
  {
    ParamEntryVal entry = par >> "OpticsIn";
    for(int i=0; i<entry.GetEntryCount(); i++)
    {
      if(i >= _viewOptics.Size()) break;
      RStringB oModelName = entry.GetEntry(i) >> "gunnerOpticsModel";
#if _VBS3 //optics model supported in viewOptics class
      if(_viewGunner._opticsModel.GetLength() > 0) oModelName = _viewGunner._opticsModel;
#endif
      _viewOptics[i]._gunnerOpticsModel = oModelName.GetLength() > 0 ? Shapes.New(::GetShapeName(oModelName), true, false) : NULL;
      if (_viewOptics[i]._gunnerOpticsModel && _viewOptics[i]._gunnerOpticsModel->NLevels() > 0)
      {
        _viewOptics[i]._gunnerOpticsModel->Level(0)->MakeCockpit();
        _viewOptics[i]._gunnerOpticsModel->OrSpecial(BestMipmap);
      }

    }
  }
  else if(_viewOptics.Size()>0)
  {//to be removed
    RStringB oModelName = par >> "gunnerOpticsModel";
#if _VBS3 //optics model supported in viewOptics class
    if(_viewGunner._opticsModel.GetLength() > 0) oModelName = _viewGunner._opticsModel;
#endif
    _viewOptics[0]._gunnerOpticsModel = oModelName.GetLength() > 0 ? Shapes.New(::GetShapeName(oModelName), true, false) : NULL;
    if (_viewOptics[0]._gunnerOpticsModel && _viewOptics[0]._gunnerOpticsModel->NLevels() > 0)
    {
      _viewOptics[0]._gunnerOpticsModel->Level(0)->MakeCockpit();
      _viewOptics[0]._gunnerOpticsModel->OrSpecial(BestMipmap);
    }
  }

  if(par.FindEntry("opticsOut"))
  {
    ParamEntryVal entry = par >> "opticsOut";
    for(int i=0; i<entry.GetEntryCount(); i++)
    {
      if(i >= _viewGunner.Size()) break;
      RStringB oModelName = entry.GetEntry(i) >> "gunnerOpticsModel";
#if _VBS3 //optics model supported in viewOptics class
      if(_viewOutOptics._opticsModel.GetLength() > 0) oModelName = _viewOutOptics._opticsModel;
#endif

      _viewGunner[i]._gunnerOpticsModel = oModelName.GetLength() > 0 ? Shapes.New(::GetShapeName(oModelName), true, false) : NULL;
      if (_viewGunner[i]._gunnerOpticsModel && _viewGunner[i]._gunnerOpticsModel->NLevels() > 0)
      {
        _viewGunner[i]._gunnerOpticsModel->Level(0)->MakeCockpit();
        _viewGunner[i]._gunnerOpticsModel->OrSpecial(BestMipmap);
      }
    }
  }
  else if(_viewGunner.Size()>0)
  {//to be removed
    RStringB oModelName = par >> "gunnerOutOpticsModel";
#if _VBS3 //optics model supported in viewOptics class
    if(_viewOutOptics._opticsModel.GetLength() > 0) oModelName = _viewOutOptics._opticsModel;
#endif

    _viewGunner[0]._gunnerOpticsModel = oModelName.GetLength() > 0 ? Shapes.New(::GetShapeName(oModelName), true, false) : NULL;
    if (_viewGunner[0]._gunnerOpticsModel && _viewGunner[0]._gunnerOpticsModel->NLevels() > 0)
    {
      _viewGunner[0]._gunnerOpticsModel->Level(0)->MakeCockpit();
      _viewGunner[0]._gunnerOpticsModel->OrSpecial(BestMipmap);
    }
  }

  // hitpoints
  ConstParamEntryPtr hitPoints = par.FindEntry("HitPoints");
  if (hitPoints)
  {  
    for (ParamClass::Iterator<> i(hitPoints); i; ++i)
    {
      ParamEntryVal entry = *i;
      if (entry.IsClass())
      {
        if (stricmp(entry.GetName(),"HitTurret")==0) _turretHit = type->AddHitPoint(entry, type->GetEntityArmor());
        else if (stricmp(entry.GetName(),"HitGun")==0) _gunHit = type->AddHitPoint(entry, type->GetEntityArmor());
        else type->AddHitPoint(entry, type->GetEntityArmor());
      }
    }
  } 
  type->CompactHitpoints();

  _gunnerGetInPos.Resize(0);
#if _HELISIM_PRECISE_INTERACTION
  _gunnerGetInPosPrecise.Resize(0);
#endif

  const Shape *memory = shape->MemoryLevel();
  if (memory)
  {
    _gunnerOpticsPos = shape->FindNamedSel(memory,(par >> "memoryPointGunnerOptics").operator RString());
    _gunnerOutOpticsPos = shape->FindNamedSel(memory,(par >> "memoryPointGunnerOutOptics").operator RString());

    #define GET_CLOSEST_POINT(array, imin, pt) \
    { \
      float min = FLT_MAX; \
      for(int j = 0; j < (array).Size(); j++) \
      { \
        float dist = ((pt) - memory->Pos((array)[j]._pos)).SquareSize(); \
        if (dist < min) { min = dist; (imin) = j;}; \
      } \
    };

    int index = shape->FindNamedSel(memory,(par >> "memoryPointsGetInGunner").operator RString());
    if (index >= 0)
    {
      const NamedSelection::Access sel(memory->NamedSel(index),shape,memory);
      for (int i=0; i<sel.Size(); i++)
      {
        if (sel[i] >= 0) 
        {
          GetInPointIndex& pt = _gunnerGetInPos.Append();
          pt._pos = sel[i];
          pt._dir = -1;
        }
      }
    }

    index = shape->FindNamedSel(memory,(par >> "memoryPointsGetInGunnerDir").operator RString());
    if (index >= 0)
    {
      const NamedSelection::Access sel(memory->NamedSel(index),shape,memory);
      for (int i=0; i<sel.Size(); i++)
      {
        if (sel[i] >= 0) 
        {          
          int imin = -1;             
          Vector3 dirpt = memory->Pos(sel[i]);
          GET_CLOSEST_POINT(_gunnerGetInPos, imin, dirpt);         
          if (imin>=0)
          {
            GetInPointIndex& pt = _gunnerGetInPos[imin];
            pt._dir = sel[i];
          }
          else
          {
            RptF("No get in gunner point in %s (%s)",cc_cast(shape->GetName()),cc_cast(type->GetName()));
          }
        }
      }
    }

    // verify that all get in/out points has direction
    for(int i = 0; i < _gunnerGetInPos.Size(); i++)
      if (_gunnerGetInPos[i]._dir < 0)
      {
        RptF("In Vehicle: %s missing gunner get in direction point", shape->Name());        
      };
  }
  else
  {
    _gunnerOpticsPos = -1;
    _gunnerOutOpticsPos = -1;
  }

  if (_gunnerGetInPos.Size() == 0)
  {
    // default fall back
    GetInPointIndex &pt = _gunnerGetInPos.Append();
    pt._pos = -1;
    pt._dir = -1;     
  }
  _gunnerGetInPos.Compact();

#if _HELISIM_PRECISE_INTERACTION
  if (memory && type->UsePreciseGetInAction())
  {
    int index = shape->FindNamedSel(memory,(par >> "memoryPointsGetInGunnerPrecise").operator RString());
    if (index >= 0)
    {
      const NamedSelection::Access sel(memory->NamedSel(index),shape,memory);
      for (int i=0; i<sel.Size(); i++)
      {
        if (sel[i] >= 0) 
        {
          GetInPointIndex& pt = _gunnerGetInPosPrecise.Append();
          pt._pos = sel[i];
          pt._dir = -1;
        }
      }
    }

    if (_gunnerGetInPosPrecise.Size() == 0)
    {
      // default fall back
      GetInPointIndex &pt = _gunnerGetInPosPrecise.Append();
      pt._pos = -1;
      pt._dir = -1;     
    }
    _gunnerGetInPosPrecise.Compact();
  }
#endif

  for (int i=0; i<_turrets.Size(); i++)
  {
    _turrets[i]->Init(shape, type);
  }

  // Reflectors
  _lights.Init(type, shape, par, type->GetEntityArmor(), type->GetDamageInfo());

#if _ENABLE_HAND_IK 
  RString handName = par >> "gunnerLeftHandAnimName";
  handName.Lower();
  if (!handName.IsEmpty()) _gunnerLeftHandIndex = shape->GetSkeleton()->FindBone(handName);
  else _gunnerLeftHandIndex = SkeletonIndexNone;

  handName = par >> "gunnerRightHandAnimName";
  handName.Lower();
  if (!handName.IsEmpty()) _gunnerRightHandIndex = shape->GetSkeleton()->FindBone(handName);
  else _gunnerRightHandIndex = SkeletonIndexNone;

  handName = par >> "gunnerLeftLegAnimName";
  handName.Lower();
  if (!handName.IsEmpty()) _gunnerLeftLegIndex = shape->GetSkeleton()->FindBone(handName);
  else _gunnerLeftLegIndex = SkeletonIndexNone;

  handName = par >> "gunnerRightLegAnimName";
  handName.Lower();
  if (!handName.IsEmpty()) _gunnerRightLegIndex = shape->GetSkeleton()->FindBone(handName);
  else _gunnerRightLegIndex = SkeletonIndexNone;
#endif 

  if (type->GetDoors())
    _gunnerDoor = type->GetDoors()->GetDoorIndex((par >> "gunnerDoor").operator RString());

  _preciseGetInOut =(GetInOutMode)(int)(par >> "preciseGetInOut");

  if(par.FindEntry("allowTabLock")) _allowTabLock = par >> "allowTabLock";
  if(par.FindEntry("showAllTargets")) _showAllTargets = (ShowAllTargets) (par>>"showAllTargets").GetInt();
}

void TurretType::Deinit(LODShape *shape, EntityType *type)
{
  _weapons.Deinit(shape);

  for (int i = 0; i < _viewOptics.Size(); i++) _viewOptics[i]._gunnerOpticsModel.Free();
  for (int i = 0; i < _viewGunner.Size(); i++) _viewGunner[i]._gunnerOpticsModel.Free();

  _gunnerGetInPos.Clear();
#if _HELISIM_PRECISE_INTERACTION
  _gunnerGetInPosPrecise.Clear();
#endif
  _opticsType.Clear();
  _opticsOutType.Clear();

  for (int i=0; i<_turrets.Size(); i++)
  {
    _turrets[i]->Deinit(shape, type);
  }
}

void TurretType::InitLevel(LODShape *shape, int level, EntityType *type)
{
  _weapons.InitLevel(shape, level);

  _gunnerProxy[level] = ManProxy();

  // convert shape proxy to my proxy
  ShapeUsed shapeLevel = shape->Level(level);
  for (int i=0; i<type->GetProxyCountForLevel(level); i++)
  {
    const ProxyObjectTyped &proxy = type->GetProxyForLevel(level, i);
    const ProxyObject &proxyObj = shapeLevel->Proxy(i);
    // first check for ProxyCrew
    ProxyCrew *proxyCrew = dyn_cast<ProxyCrew, Object>(proxy.obj);
    if (proxyCrew)
    {
      // check crew type
      CrewPosition pos = proxyCrew->Type()->GetCrewPosition();
      int id = proxy.id;
      if (id < 1)
      {
        Fail("Bad cargo proxy index");
        continue;
      }
      if (pos == _proxyType && id == _proxyIndex)
      {
        // proxy found
        Matrix4 &trans = _gunnerProxy[level].transform;
        trans = proxy.obj->FutureVisualState().Transform();
        LODShape *pshape = proxy.obj->GetShape();
        // reset position BC correction
        Vector3 offset = VZero;
        if (pshape) offset = -pshape->BoundingCenter();
        trans.SetPosition(trans.FastTransform(offset));
        // rotate around y-axis (shape is reversed)
        trans.SetOrientation(trans.Orientation() * Matrix3(MScale, -1, 1, -1));
        _gunnerProxy[level]._proxy = &proxyObj;
        break;
      }
    }
  }

  // check if turret structure in the config match the structure in the skeleton
  if (level == shape->FindMemoryLevel())
  {
    // check in memory
    const Skeleton *sk = shape->GetSkeleton();

    SkeletonIndex parent = GetBodyBone(type, level);
    if (parent != SkeletonIndexNone)
    {
      for (int i=0; i<_turrets.Size(); i++)
      {
        SkeletonIndex child = _turrets[i]->GetBodyBone(type, level);
        if (child != SkeletonIndexNone)
        {
          if (sk->GetParent(child) != parent)
          {
            RptF(
              "Type %s, model %s - structure of turrets in config does not match the skeleton",
              cc_cast(type->GetName()), cc_cast(shape->GetName()));
          }
        }
      }
    }
  }

  for (int i=0; i<_turrets.Size(); i++)
  {
    _turrets[i]->InitLevel(shape, level, type);
  }

  _lights.InitLevel(shape, level);
}

void TurretType::DeinitLevel(LODShape *shape, int level, EntityType *type)
{
  _weapons.DeinitLevel(shape, level);

  _gunnerProxy[level] = ManProxy();
  for (int i=0; i<_turrets.Size(); i++)
  {
    _turrets[i]->DeinitLevel(shape, level, type);
  }

  _lights.DeinitLevel(shape, level);
}

bool TurretType::ForEachTurret(const EntityAIType *entityType, TurretPath &path, ITurretTypeFunc &func) const
{
  if (func(entityType, *this, path)) return true;

  int n = path.Size();
  for (int i=0; i<_turrets.Size(); i++)
  {
    path.Resize(n + 1);
    path[n] = i;
    if (_turrets[i]->ForEachTurret(entityType, path, func)) return true;
  }
  return false;
}

const TurretType *TurretType::GetTurret(const int *path, int size) const
{
  if (size == 0) return this;
  int i = path[0];
  if (i < 0 || i >= _turrets.Size())
  {
    LogF("Invalid turret path");
    return NULL;
  }
  return _turrets[i]->GetTurret(path + 1, size - 1);
}


TurretVisualState::TurretVisualState(TurretType const& type, TransportType const& ownerType)
:_xRot(0),_yRot(0),
_IKIdent(true),
_leftHandIK(M4Identity),
_rightHandIK(M4Identity),
_leftLegIK(M4Identity),
_rightLegIK(M4Identity),
_crewHidden(ownerType.GetHideProxyInCombat() && type._canHideGunner ? 1.0f : 0.0f)
{
  Init(type);
  int n = type._turrets.Size();
  _turretVisualStates.Realloc(n);
  _turretVisualStates.Resize(n);
  for (int i=0; i<n; i++)
    _turretVisualStates[i] = new TurretVisualState(*type._turrets[i], ownerType);
}

TurretVisualStateArray::TurretVisualStateArray(const TurretVisualStateArray &src)
{
  Realloc(src.Size());
  Resize(src.Size());
  for (int i=0; i<src.Size(); i++) Set(i)=new TurretVisualState(*src.Get(i));
}

void TurretVisualStateArray::operator = (const TurretVisualStateArray &src)
{
  Assert(Size()==src.Size());
  // overwrite values, don't reallocate the tree structure
  for (int i=0; i<src.Size(); i++) *Set(i)=*src.Get(i);
}



Turret::Turret()
:
#pragma warning(push)
#pragma warning(disable:4355)
_weaponsState(*this),
#pragma warning(pop)
_yRotWanted(0),
_xRotWanted(0),
_zoom(),
_xSpeed(0),
_ySpeed(0),
_servoVol(0),
_gunStabilized(false),
_owner(NULL),
_visionMode(VMNormal),
_tiIndex(0),
_locked(false),
_crewHiddenWanted(false),
_currentViewGunnerMode(0),
_currentViewOpticsMode(0),
_gunPosIndexToogle(0),
_heatFactor(0),
_searchLightOn(false)
#if _VBS3
  ,_type(NULL)
  ,_disableGunnerInput(false)
  ,_overrideTurret(NULL)
  ,_overriddenByTurret(NULL)
  ,_lockOnObj(NULL)
  ,_lockOnPos(VZero)
  ,_showOverrideOptics(false)
  ,_overrideStartTime(0)
  ,_tiModeIndex(0)
  ,_tiModeOutIndex(0)
  ,_visionMode(VMInvalid)
  ,_visionOutMode(VMInvalid)
#endif
#if _VBS3_LASE_LEAD
  ,_laseDistance(-1)
  ,_laserWeaponAimed(-1)
  ,_laseOffsetX(0), _laseOffsetXWanted(0)
  ,_laseOffsetY(0)
#endif

{  
  _local = true;
}

Turret::Turret(const TurretType &type, Transport *owner)
: 
#pragma warning(push)
#pragma warning(disable:4355)
_weaponsState(*this),
#pragma warning(pop)
_yRotWanted(0),
_xRotWanted(0),
_zoom(),
_xSpeed(0),
_ySpeed(0),
_servoVol(0),
_gunStabilized(false),
_owner(owner),
_locked(false),
_visionMode(VMNormal),
_tiIndex(0),
_crewHiddenWanted(owner->Type()->GetHideProxyInCombat() && type._canHideGunner ? 1.0f : 0.0f),
_currentViewGunnerMode(0),
_currentViewOpticsMode(0),
_heatFactor(0),
_gunPosIndexToogle(0),
_searchLightOn(false)
#if _VBS3
  ,_type(&type)
  ,_disableGunnerInput(false)
  ,_overrideTurret(NULL)
  ,_overriddenByTurret(NULL)
  ,_lockOnObj(NULL)
  ,_lockOnPos(VZero)
  ,_showOverrideOptics(false)
  ,_overrideStartTime(0)
  ,_tiModeIndex(0)
  ,_tiModeOutIndex(0)
  ,_visionMode(VMInvalid)
  ,_visionOutMode(VMInvalid)
#endif
#if _VBS3_LASE_LEAD
  ,_laseDistance(-1)
  ,_laserWeaponAimed(-1)
  ,_laseOffsetX(0), _laseOffsetXWanted(0)
  ,_laseOffsetY(0)
#endif
{
  _local = true;
  Init(type,owner->FutureVisualState().Orientation());
  _zoom.Init(&type._viewOptics[_currentViewOpticsMode]);

  // Turrets
  int n = type._turrets.Size();
  _turrets.Realloc(n);
  _turrets.Resize(n);
  for (int i=0; i<n; i++)
  {
    _turrets[i] = new Turret(*type._turrets[i], owner);
  }

  _oldviewModes.Resize(type._viewOptics.Size());
  for (int i = 0; i<_oldviewModes.Size(); i++) _oldviewModes[i]._visionMode = VMInvalid;
  _oldGunnerModes.Resize(type._viewGunner.Size());
  for (int i = 0; i<_oldGunnerModes.Size(); i++) _oldGunnerModes[i]._visionMode = VMInvalid;
}

// IWeaponsHolder implementation
void Turret::Compact()
{
  _weapons.Compact();
  _magazines.Compact();
}

bool Turret::ForEachWeapon(const std::function<bool(Weapon *weapon)> &func)
{
  for (int i=0; i<_weapons.Size(); i++)
  {
    Weapon *weapon = _weapons[i];
    if (!weapon) continue;
    if (func(weapon)) return true;
  }
  return false;
}

bool Turret::ForEachWeapon(const std::function<bool(const Weapon *weapon)> &func) const
{
  for (int i=0; i<_weapons.Size(); i++)
  {
    const Weapon *weapon = _weapons[i];
    if (!weapon) continue;
    if (func(weapon)) return true;
  }
  return false;
}

bool Turret::AddWeapon(EntityAIFull *entity, Weapon *weapon)
{
  weapon->ItemMoveOut(entity);
  _weapons.Add(weapon);
  return true;
}

bool Turret::RemoveWeapon(Weapon *weapon)
{
  // find and remove weapon
  if (!_weapons.Delete(weapon)) return false; // not found
  // unassign the weapon, but leave it in the world
  weapon->CancelHierarchyParent(_owner);
  return true;
}

bool Turret::DeleteWeapon(RStringB name)
{
  // find and delete the weapon
  for (int i=0; i<_weapons.Size(); i++)
  {
    Weapon *weapon = _weapons[i];
    if (!weapon) continue;
    const WeaponType *type = weapon->Type();
    if (type && stricmp(type->GetName(), name) == 0)
    {
      // remove from the world
      weapon->ItemDestroy();
      // and the container
      _weapons.Delete(i);
      
      return true;
    }
  }
  return false; // not found
}

void Turret::DeleteWeapons(const std::function<bool(const Weapon *weapon)> &func)
{
  for (int i=_weapons.Size()-1; i>=0; i--) // go reverse to avoid problems with deletion
  {
    Weapon *weapon = _weapons[i];
    if (!weapon) continue;

    if (func(weapon))
    {
      // remove from the world
      weapon->ItemDestroy();
      // and the container
      _weapons.DeleteAt(i);
    }
  }
}

void Turret::DeleteAllWeapons()
{
  for (int i=0; i<_weapons.Size(); i++)
  {
    Weapon *weapon = _weapons[i];
    if (!weapon) continue;
    // remove from the world
    weapon->ItemDestroy();
  }
  _weapons.Clear();
}

bool Turret::ForEachMagazine(const std::function<bool(Magazine *)> &func)
{
  for (int i=0; i<_magazines.Size(); i++)
  {
    Magazine *magazine = _magazines[i];
    if (!magazine) continue;
    if (func(magazine)) return true;
  }
  return false;
}

bool Turret::AddMagazine(EntityAIFull *entity, Magazine *magazine)
{
  magazine->ItemMoveOut(entity);
  _magazines.Add(magazine);
  return true;
}

bool Turret::RemoveMagazine(Magazine *magazine)
{
  // find and remove magazine
  if (!_magazines.Delete(magazine)) return false; // not found
  // unassign the weapon, but leave it in the world
  magazine->CancelHierarchyParent(_owner);
  return true;
}

bool Turret::DeleteMagazine(RStringB name)
{
  // find and delete the magazine
  for (int i=0; i<_magazines.Size(); i++)
  {
    Magazine *magazine = _magazines[i];
    if (!magazine) continue;
    const MagazineType *type = magazine->Type();
    if (type && stricmp(type->GetName(), name) == 0)
    {
      // remove from the world
      magazine->ItemDestroy();
      // and the container
      _magazines.Delete(i);
      
      return true;
    }
  }
  return false; // not found
}

void Turret::DeleteMagazines(const std::function<bool(const Magazine *magazine)> &func)
{
  for (int i=_magazines.Size()-1; i>=0; i--) // go reverse to avoid problems with deletion
  {
    Magazine *magazine = _magazines[i];
    if (!magazine) continue;

    if (func(magazine))
    {
      // remove from the world
      magazine->ItemDestroy();
      // and the container
      _magazines.DeleteAt(i);
    }
  }
}

void Turret::DeleteAllMagazines()
{
  for (int i=0; i<_magazines.Size(); i++)
  {
    Magazine *magazine = _magazines[i];
    if (!magazine) continue;

    // remove from the world
    magazine->ItemDestroy();
  }
  _magazines.Clear();
}

WeaponModePtr Turret::GetCurrentWeapon() const
{
  WeaponModePtr ptr;
  
  if (_currentWeapon)
  {
    ptr._weapon = _currentWeapon;
    ptr._muzzle = _currentWeapon->GetCurrentMuzzle();
    if (ptr._muzzle >= 0 && ptr._muzzle < ptr._weapon->NMuzzles())
    {
      const Muzzle &muzzle = ptr._weapon->GetMuzzle(ptr._muzzle);
      ptr._mode = muzzle._currentMode;
    }
  }

  return ptr;
}

void Turret::SetCurrentWeapon(const WeaponModePtr &ptr)
{
  _currentWeapon = ptr._weapon;
  if (ptr.IsValid())
  {
    Muzzle &muzzle = ptr._weapon->SetCurrentMuzzle(ptr._muzzle);
    muzzle._currentMode = ptr._mode;
  }
}

#if _VBS3
/// Functor to get the turret based on a index
class GetTurretPathFromClassName:public ITurretTypeFunc
{
protected:
  const RString& _className;
  const TurretType &_type;
public:
  TurretPath _path;

  GetTurretPathFromClassName(const RString& className, const TurretType &type)
    :_className(className),_type(type)
  {}

  virtual bool operator () (const EntityAIType *entityType, const TurretType &turretType, TurretPath &path) const
  {
    if (strcmpi(_className, turretType._par->GetName()) == 0)
    {
      const_cast<GetTurretPathFromClassName&>(*this)._path = path;
      return true;
    }
    return false; // continue
  }
};
// establish the Override linking between turrets for Commander Override
void Turret::SetOverrideTurretLink()
{
  Assert(_type);
  if(_type->_overrideTurretName.GetLength() > 0)
  {
    Transport* owner = _owner.GetLink();
    Assert(owner);
    GetTurretPathFromClassName func(_type->_overrideTurretName, *_type);
    if(owner->Type()->ForEachTurret(func))
    {
      Turret* turret = owner->GetTurret(func._path.Data(),func._path.Size());
      if(turret)
        _overrideTurret = turret;
    }
  }
};

void Turret::StartOverride()
{
  Assert(_gunner)
    Assert(_overrideTurret)

    if(_overrideTurret->IsLocal())
      _overrideTurret->_overriddenByTurret = this;
    else
      GetNetworkManager().AskCommanderOverride(_overrideTurret, this);
  _overrideStartTime = ::GetTickCount();
}

void Turret::StopOverride()
{
  Assert(_gunner)
    Assert(_overrideTurret)

    if(_overrideTurret->IsLocal())
      _overrideTurret->_overriddenByTurret = NULL;
    else
    {
      GetNetworkManager().AskCommanderOverride(_overrideTurret, NULL);
      LogF("[m] Error Overriding turret should be local!");
    }
    _overrideStartTime = 0;
}

bool Turret::UpdateOverrideStatus()
{
  if(!_overrideTurret)
    return false;

  bool overrideActive = _overrideTurret->_overriddenByTurret == this;

  bool manual = GLOB_WORLD->PlayerManual() && GLOB_WORLD->PlayerOn();

  if (_gunner && _gunner->Brain()->LSIsAlive())
  {
    bool playerIsGunner = manual && _gunner && _gunner->Brain() == GLOB_WORLD->PlayerOn()->Brain(); 
    if(playerIsGunner)
    {
      if(GInput.GetAction(UACommanderOverride,false) > 0.0f)
      {
        if(!overrideActive)
          StartOverride();
        else
          _overrideStartTime = ::GetTickCount();
      }
      else
      {
        if(overrideActive)
        {
          if(::GetTickCount() - _overrideStartTime > 200) //accept 200msec delay
          {
            StopOverride();
            return false;
          }
        }
      }
    }//playerIsGunner
  }
  else //No gunner or not alive
  {
    if(overrideActive) StopOverride();
    return false;
  }
  return overrideActive;
}

VisionMode Turret::NextVisionMode()
{
  const ViewPars* viewOptics = GetCurrentViewPars();

  if (_owner && _owner->Type()->GetHideProxyInCombat() && GetGunnerHidden() < 0.5)
  {
    viewOptics->NextVisionMode(_visionOutMode, _tiModeOutIndex);
    return _visionOutMode;
  }
  else
  {
    viewOptics->NextVisionMode(_visionMode, _tiModeIndex);
    return _visionMode;
  }
}

int Turret::GetTiModus()
{
  if (_owner && _owner->Type()->GetHideProxyInCombat() && GetGunnerHidden() < 0.5)
  {
    Assert(_tiModeIndex <= _type->_viewOutOptics._thermalModes.Size())
      return _type->_viewOutOptics._thermalModes[_tiModeOutIndex];
  }
  else
  {
    Assert(_tiModeIndex <= _type->_viewOptics._thermalModes.Size())
      return _type->_viewOptics._thermalModes[_tiModeIndex];
  }
}

bool Turret::ApplyVisionMode()
{
  bool turnedOut = (_owner && _owner->Type()->GetHideProxyInCombat() && GetGunnerHidden() < 0.5);
  VisionMode &vm = turnedOut ? _visionOutMode : _visionMode;
  int &ti = turnedOut ? _tiModeOutIndex : _tiModeIndex;

  return GetCurrentViewPars()->ApplyVisionMode(vm, ti);
}

const ViewPars* Turret::GetCurrentViewPars() const
{
  if (_owner && _owner->Type()->GetHideProxyInCombat() && GetGunnerHidden() < 0.5)
    return &_type->_viewOutOptics;
  else
    return &_type->_viewOptics;
}

VisionMode Turret::GetVisionMode() const 
{
  if (_owner && _owner->Type()->GetHideProxyInCombat() && GetGunnerHidden() < 0.5)
    return _visionOutMode;
  else
    return _visionMode;
}
#endif //_VBS3

bool Turret::HasVisionModes(const TurretType *type) const
{
  const ViewPars* vPars = GetCurrentViewPars(type);

  if (vPars)
  {
    return vPars->HasVisionModes();
  }

  return false;
}

int Turret::VisionModesCount(const TurretType *type) const
{
  const ViewPars* vPars = GetCurrentViewPars(type);

  if (vPars)
  {
    return vPars->VisionModesCount();
  }
  return 0;
}

bool Turret::HasThermalModes(const TurretType *type) const
{
  const ViewPars* vPars = GetCurrentViewPars(type);

  if (vPars)
  {
    return vPars->HasThermalModes();
  }

  return false;
}

const ViewPars* Turret::GetCurrentViewPars(const TurretType *type, bool allTurrets) const
{
  if (!type->_viewOptics[_currentViewOpticsMode].HasVisionModes()) return NULL;

  if (_owner.NotNull() && ( GWorld->GetCameraType() == CamGunner || allTurrets))
    return &type->_viewOptics[_currentViewOpticsMode];
  
  return NULL;
}

VisionMode Turret::GetVisionModePars(int &tiIndex) const
{
  tiIndex = _tiIndex;
  return _visionMode;
}

bool Turret::NextVisionMode(const TurretType *type)
{
  const ViewPars* vPars = GetCurrentViewPars(type);

  if (vPars)
  {
    VisionMode currVMode = _visionMode;
    vPars->NextVisionMode(_visionMode, _tiIndex);
    return _visionMode != currVMode;
  }

  return false;
}

void Turret::NextOpticsMode(Person *person,const TurretType *type)
{
  Object::Zoom *zoom = GetZoom();
  VisionMode oldVMode = _visionMode;
  int tiIndex = _tiIndex;
  if(GWorld->GetCameraType() == CamGunner ) //inside
  { //save old optics settings
    if(_oldviewModes.Size() != type->_viewOptics.Size()) return;
    _oldviewModes[_currentViewOpticsMode]._tiIndex = _tiIndex;
    _oldviewModes[_currentViewOpticsMode]._visionMode = _visionMode;
    //select new optics
    _currentViewOpticsMode = std::min((_currentViewOpticsMode+1), type->_viewOptics.Size()-1);
    zoom->Init(&type->_viewOptics[_currentViewOpticsMode]);
    if(!CheckVisionMode(oldVMode,tiIndex,type,type->_viewOptics[_currentViewOpticsMode]))
    {//if new optics does not support current mode, change to old settings
      if(_oldviewModes[_currentViewOpticsMode]._visionMode == VMInvalid) NextVisionMode(type);
      else
      {
        _tiIndex = _oldviewModes[_currentViewOpticsMode]._tiIndex;
        _visionMode = _oldviewModes[_currentViewOpticsMode]._visionMode;
      }
    }
  }
  else //outside
  { //save old optics settings
    if(_oldGunnerModes.Size() != type->_viewGunner.Size()) return;
    _oldGunnerModes[_currentViewGunnerMode]._tiIndex = _tiIndex;
    _oldGunnerModes[_currentViewGunnerMode]._visionMode = _visionMode;
    //select new optics
    _currentViewGunnerMode = std::min((_currentViewGunnerMode+1), type->_viewGunner.Size()-1);
    zoom->Init(&type->_viewGunner[_currentViewGunnerMode]);
    if(!CheckVisionMode(oldVMode,tiIndex,type,type->_viewGunner[_currentViewGunnerMode]))
    {//if new optics does not support current mode, change to old settings
      if(_oldGunnerModes[_currentViewGunnerMode]._visionMode == VMInvalid) NextVisionMode(type);
      else
      {
        _tiIndex = _oldGunnerModes[_currentViewGunnerMode]._tiIndex;
        _visionMode = _oldGunnerModes[_currentViewGunnerMode]._visionMode;
      }
    }
  }
}

void Turret::PreviousOpticsMode(Person *person, const TurretType *type)
{
  Object::Zoom *zoom = GetZoom();
  VisionMode oldVMode = _visionMode;
  int tiIndex = _tiIndex;
  if(GWorld->GetCameraType() == CamGunner ) //inside
  { //save old optics settings
    if(_oldviewModes.Size() != type->_viewOptics.Size()) return;
    _oldviewModes[_currentViewOpticsMode]._tiIndex = _tiIndex;
    _oldviewModes[_currentViewOpticsMode]._visionMode = _visionMode;
    //select new optics
    _currentViewOpticsMode = std::max((_currentViewOpticsMode-1), 0);
    zoom->Init(&type->_viewOptics[_currentViewOpticsMode]);
    if(!CheckVisionMode(oldVMode,tiIndex,type,type->_viewOptics[_currentViewOpticsMode]))
    {//if new optics does not support current mode, change to old settings
      if(_oldviewModes[_currentViewOpticsMode]._visionMode == VMInvalid) NextVisionMode(type);
      else
      {
        _tiIndex = _oldviewModes[_currentViewOpticsMode]._tiIndex;
        _visionMode = _oldviewModes[_currentViewOpticsMode]._visionMode;
      }
    }
  }
  else //outside
  { //save old optics settings
    if(_oldGunnerModes.Size() != type->_viewGunner.Size()) return;
    _oldGunnerModes[_currentViewGunnerMode]._tiIndex = _tiIndex;
    _oldGunnerModes[_currentViewGunnerMode]._visionMode = _visionMode;
    //select new optics
    _currentViewGunnerMode = std::max((_currentViewGunnerMode-1), 0);
    zoom->Init(&type->_viewGunner[_currentViewGunnerMode]);
    if(!CheckVisionMode(oldVMode,tiIndex,type,type->_viewGunner[_currentViewGunnerMode]))
    {//if new optics does not support current mode, change to old settings
      if(_oldGunnerModes[_currentViewGunnerMode]._visionMode == VMInvalid) NextVisionMode(type);
      else
      {
        _tiIndex = _oldGunnerModes[_currentViewGunnerMode]._tiIndex;
        _visionMode = _oldGunnerModes[_currentViewGunnerMode]._visionMode;
      }
    }
  }
}

bool Turret::CheckVisionMode(VisionMode oldVMode, int tiIndex, const TurretType *type, ViewPars view)
{
  bool hasMode = false;
  for (int i = 0; i< view._visionModes.Size() ; i++)
  {
    if(view._visionModes[i] == oldVMode) hasMode = true;
  }
  if(!hasMode) return false;

  for (int i = 0; i< view._thermalModes.Size() ; i++)
  {
    if(view._thermalModes[i] == tiIndex) hasMode = true;
  }
  return hasMode;
}

void Turret::ZeroingUp(const TurretType *type)
{
    if(_distanceIndex < type->_discreteDistance.Size()-1) _distanceIndex++;
    else  _distanceIndex = type->_discreteDistance.Size()-1;
}

void Turret::ZeroingDown(const TurretType *type)
{
  if(_distanceIndex > 0) _distanceIndex--;
}

Turret::~Turret()
{
}

void Turret::Init(const TurretType &type, Matrix3Par parent)
{
  _xRotWanted = type._initElev;
  _yRotWanted = type._initTurn;

  const ViewPars* vPars = GetCurrentViewPars(&type, true);
  if (vPars) 
  {
    vPars->GetInitVisionMode(_visionMode, _tiIndex);
  }

  _stabilizeTo.SetDirectionAndUp(Vector3(0,0,0),Vector3(0,0,0));

  _distanceIndex = type._discreteDistanceInitIndex;

  // create reflectors
  _lights.Create(_owner, type._lights);

}

void Turret::Animate(const TurretType &type, AnimationContext &animContext, LODShape *shape, int level, bool setEngineStuff, Object *parentObject)
{
  if (_weaponsState.ShowFire())
  {
    type._weapons._animFire.Unhide(animContext, shape, level);
    type._weapons._animFire.SetPhase(animContext, shape, level, _weaponsState._mGunFirePhase);
  }
  else
  {
    type._weapons._animFire.Hide(animContext, shape, level);
  }

  // animate turrets
  for (int i=0; i<_turrets.Size(); i++)
  {
    TurretType *turretType = type._turrets[i];
    Turret *turret = _turrets[i];
    turret->Animate(*turretType, animContext, shape, level, setEngineStuff, parentObject);
  }

  // reflectors
  _lights.Animate(animContext, level, _owner, this, type._lights);
}

void Turret::Deanimate(const TurretType &type, LODShape *shape, int level, bool setEngineStuff)
{
  // deanimate turrets
  for (int i=0; i<_turrets.Size(); i++)
  {
    TurretType *turretType = type._turrets[i];
    Turret *turret = _turrets[i];
    turret->Deanimate(*turretType, shape, level, setEngineStuff);
  }
}


void Turret::HideGunner(float hide) 
{
  if (_crewHiddenWanted == hide ) return;
  _crewHiddenWanted = hide;
}


const Turret *Turret::GetTurret(const int *path, int size) const
{
  if (size == 0) return this;
  int i = path[0];
#if _VBS3
  if( i >= _turrets.Size() ) return NULL;
#endif
  return _turrets[i]->GetTurret(path + 1, size - 1);
}

Turret *Turret::GetTurret(const int *path, int size)
{
  if (size == 0) return this;
  int i = path[0];
  if (i < 0 || i >= _turrets.Size())
  {
    LogF("Invalid turret path");
    return NULL;
  }
  return _turrets[i]->GetTurret(path + 1, size - 1);
}

Turret *Turret::CreateObject(ParamArchive &ar)
{
  return new Turret();
}

LSError Turret::Serialize(ParamArchive &ar)
{
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    _owner = reinterpret_cast<Transport *>(ar.GetParams());
    Assert(_owner);
  }
  
  // get the equivalent turret in FutureVisualState()
  AutoArray<int> path; _owner->GetTurretPath(path, this);
  TurretVisualState& vs = unconst_cast( *_owner->FutureVisualState().GetTurret(path.Data(), path.Size()) );

  CHECK(ar.Serialize("Turrets", _turrets, 1))

  CHECK(ar.SerializeRefs("Weapons", _weapons, 1));
  CHECK(ar.SerializeRefs("Magazines", _magazines, 1));
  CHECK(ar.SerializeRef("currentWeapon", _currentWeapon, 1))
  CHECK(_weaponsState.Serialize(_owner, ar))

  SerializeBitBool(ar, "gunStabilized", _gunStabilized, 1, false)
  CHECK(ar.Serialize("yRot", vs._yRot, 1, 0))
  CHECK(ar.Serialize("xRot", vs._xRot, 1, 0))
  CHECK(ar.Serialize("yRotWanted", _yRotWanted, 1, 0))
  CHECK(ar.Serialize("xRotWanted", _xRotWanted, 1, 0))
  CHECK(ar.Serialize("xSpeed", _xSpeed, 1, 0))
  CHECK(ar.Serialize("ySpeed", _ySpeed, 1, 0))

  CHECK(ar.Serialize("locked", _locked, 1, false))

  CHECK(ar.Serialize("crewHidden", vs._crewHidden, 1, 1.0f))
  CHECK(ar.Serialize("crewHiddenWanted", _crewHiddenWanted, 1, 1.0f))

  CHECK(ar.Serialize("zoom", _zoom, 1));

  CHECK(ar.Serialize("distanceIndex",_distanceIndex, 1));

  CHECK(ar.SerializeRef("Gunner", _gunner, 1))
  CHECK(ar.SerializeRef("GunnerAssigned", _gunnerAssigned, 1))

  CHECK(ar.SerializeEnum("visionMode", _visionMode, 1, ENUM_CAST(VisionMode, VMNormal)))
  CHECK(ar.Serialize("tiIndex", _tiIndex, 1, 0))
  CHECK(ar.Serialize("heatFactor", _heatFactor, 1, 0))

  CHECK(ar.Serialize("oldviewModes", _oldviewModes, 1))
  CHECK(ar.Serialize("oldGunnerModes", _oldGunnerModes, 1))

  CHECK(::Serialize(ar, "stabilizeTo", _stabilizeTo, 1))

  CHECK(::Serialize(ar, "searchLightOn", _searchLightOn, 1, false))
  // TODO: _weaponsState

  return LSOK;
}

Turret *Turret::LoadRef(ParamArchive &ar)
{
  Transport *owner = dyn_cast<Transport>(Transport::LoadRef(ar));
  if (!owner) return NULL;
  AutoArray<int, MemAllocLocal<int, 16> > path;
  if (ar.SerializeArray("path", path, 1) != LSOK) return NULL;
  return owner->GetTurret(path.Data(), path.Size());
}

LSError Turret::SaveRef(ParamArchive &ar)
{
  CHECK(_owner->SaveRef(ar));
  AutoArray<int, MemAllocLocal<int, 16> > path;
  _owner->GetTurretPath(path, this);
  CHECK(ar.SerializeArray("path", path, 1))
  return LSOK;
}

void Turret::Sound(
  Transport *parent,
  const TurretType &type, bool inside, float deltaT,
  FrameBase &pos, Vector3Val speed // parent position
)
{
  const SoundPars &pars=type._servoSound;
  float vol = pars.vol *_servoVol;

  if (!inside)
  {
    // assume servos can be heard better inside
    vol *= 0.3;
  }
  if (_servoVol>0.001f && pars.name.GetLength()>0 && 
    (inside || GSoundScene->CanBeAudible(pos.Position(), pars.distance)) )
  {
    if( !_servoSound)
    {
      _servoSound=GSoundScene->OpenAndPlay(pars.name,parent,inside,pos.Position(),speed, vol, pars.freq, pars.distance);
    }
    if( _servoSound )
    {
      _servoSound->SetVolume(vol, pars.freq); // volume, frequency
      _servoSound->SetPosition(pos.Position(),speed);
    }
  }
  else
  {
    _servoSound.Free();
  }

  for (int i=0; i<_turrets.Size(); i++)
  {
    TurretType *turretType = type._turrets[i];
    Turret *turret = _turrets[i];
    turret->Sound(parent,*turretType, inside, deltaT, pos, speed);
  }
}

void Turret::UnloadSound()
{
  _servoSound.Free();

  for (int i=0; i<_turrets.Size(); i++)
  {
    Turret *turret = _turrets[i];
    turret->UnloadSound();
  }
}

RString Turret::GunnerAction(TurretVisualState &vs, const TurretType &type) const
{
  if (vs.IsGunnerHidden()) return type._gunnerInAction;
  return type._gunnerAction;
}

bool Turret::ForEachTurret(Transport *vehicle, const TurretType *type, ITurretFunc &func) const
{
  TurretContext context;
  context._turret = unconst_cast(this);
  context._turretType = type;
  context._gunner = _gunner;
  context._weapons = unconst_cast(&_weaponsState);
  if (func(vehicle, context)) return true;

  int n = _turrets.Size();
  for (int i=0; i<n; i++)
  {
    if (_turrets[i]->ForEachTurret(vehicle, type->_turrets[i], func)) return true;
  }
  return false;
}

bool Turret::ForEachTurret(TurretVisualState& vs, Transport *vehicle, const TurretType *type, ITurretFuncV &func) const
{
  TurretContextV context;
  context._turret = unconst_cast(this);
  context._turretType = type;
  context._gunner = _gunner;
  context._weapons = unconst_cast(&_weaponsState);
  context._turretVisualState = &vs;
  if (func(vehicle, context)) return true;

  int n = _turrets.Size();
  if (n > 0)
  {
    Assert(vs._turretVisualStates.Size() == n);
    for (int i=0; i<n; i++)
    {
      if (_turrets[i]->ForEachTurret(*vs._turretVisualStates[i], vehicle, type->_turrets[i], func)) return true;
    }
  }
  return false;
}

bool Turret::ForEachTurretEx(TurretVisualState& vs, Transport *vehicle, const TurretType *type, Matrix3Val parentTrans, ITurretFuncEx &func) const
{
  TurretContextEx context;
  context._turret = unconst_cast(this);
  context._turretType = type;
  context._gunner = _gunner;
  context._weapons = unconst_cast(&_weaponsState);
  context._parentTrans = parentTrans;
  context._turretVisualState = &vs;
  if (func(vehicle, context)) return true;

  int n = _turrets.Size();
  if (n > 0)
  {
    Matrix3 trans = parentTrans * vs.TurretOrientation();
    Assert(vs._turretVisualStates.Size() == n);
    for (int i=0; i<n; i++)
    {
      if (_turrets[i]->ForEachTurretEx(*vs._turretVisualStates[i], vehicle, type->_turrets[i], trans, func)) return true;
    }
  }
  return false;
}

void Turret::SimulateEffects(TurretVisualState &vs, const Transport *obj, const TurretType &type, float deltaT)
{
  if (
    _weaponsState._gunFire.Active() || _weaponsState._gunClouds.Active() ||
    _weaponsState._mGunClouds.Active() || _weaponsState._mGunFire.Active())
  {
    int posArraySize = type._gunPosArray.Size();
    bool isTooglingOk = posArraySize>0 && posArraySize>_gunPosIndexToogle;
    //DoAssert(isTooglingOk);
    
    // TODO: avoid recursive calculation of GunTurretTransform using transform propagation
    Matrix4Val gunTransform = obj->GunTurretTransform(obj->FutureVisualState(), type);
    Matrix4Val toWorld = obj->FutureVisualState().Transform() * gunTransform;
    Vector3Val dir = toWorld.Direction();
    Vector3 firePos(VFastTransform, toWorld, type._pos0 + 0.6f * type._dir0);
    Vector3 smokePos(VFastTransform, toWorld, type._pos0 + 0.6f * type._dir0);
    Vector3 gunPos(VFastTransform, toWorld, 
      isTooglingOk? type._gunPosArray[_gunPosIndexToogle] : type._gunPos);
    _weaponsState._gunFire.Simulate(firePos, obj->FutureVisualState().Speed() * 0.85f + dir * 20, 0.8f, deltaT);
#if _VBS2 //faster MGun clouds
    _weaponsState._mGunClouds.Simulate(gunPos, obj->FutureVisualState().Speed() * 0.7f + dir * 3, 0.35f, deltaT);
#else
    _weaponsState._mGunClouds.Simulate(gunPos, obj->FutureVisualState().Speed() * 0.7f + dir * 0.1f, 0.35f, deltaT);
#endif
    _weaponsState._mGunFire.Simulate(gunPos, deltaT);
  }
}

bool Turret::Simulate(TurretVisualState& vs, const Transport *obj, const TurretType &type, float deltaT, SimulationImportance prec, Matrix3Val parentTrans)
{
  if (SimulateHatch(vs, deltaT, prec) && _gunner)
  {
    _gunner->SwitchAction(GunnerAction(vs, type));
  }

  // simulate owned weapons
  ForEachWeapon([deltaT, prec](Weapon *weapon) -> bool
  {
    weapon->Simulate(deltaT, prec);
    return false; // continue
  });

  static float hoursToHeatUp = 5.0f / 60.0f; // 5 minutes to cool down
  const float invSecToHeatUp = 1.0f / (hoursToHeatUp * 60.0f * 60.0f);  
  _heatFactor -= deltaT * invSecToHeatUp;
  saturate(_heatFactor, 0.0f, 1.0f);

  bool moved = false;


  // simulate itself
  AIBrain *unit = _gunner ? _gunner->Brain() : NULL;

  if(!unit || (_owner && _gunner == _owner->Copilot()))
  {
    Stop(type, parentTrans);
    if(_owner && _gunner == _owner->Copilot()) _stabilizeTo = _owner->FutureVisualState().Transform().Orientation();
  }
  else
  {
    if (obj->GetHit(type._gunHit) > 0.9f)
    {
      GunBroken(type, parentTrans);
    }

#if _VBS3
    if(_lockOnPos != VZero || !_lockOnObj.IsNull())
    {
      Vector3 pos = _lockOnPos != VZero ? _lockOnPos : _lockOnObj->Position();
      AimWorld(*_type, pos - _owner->Position());
    }
#endif

    // check if driver hatch is closed
    // if not, main turret must be locked
    // note: this probably does not apply to some tanks (M113, BMP)
    if ((type._lockWhenDriverOut && obj->Type()->HasDriver() && obj->GetDriverHidden() < 0.99f) || (!(type._lockWhenVehicleSpeed < 0.0f) && (obj->FutureVisualState().Speed().SquareSize() > Square(type._lockWhenVehicleSpeed))))
    {
      LockForward(type);
    }
    else if (!type.OutGunnerMayFire(_gunner) && vs._crewHidden <= 0.5)
    {
      LockForward(type);
    }
    else if (obj->GetHit(type._turretHit) > 0.9f)
    {
      // break turrets recursively
      TurretBroken(vs, type, parentTrans);
    }
    else
    {
      SetStabilization(true, parentTrans);
    }

    Matrix3 oldTurretTrans = parentTrans * vs.TurretOrientation();
    if (MoveWeapons(vs, type, unit, deltaT, parentTrans) && type._startEngine)
    {
      moved = true;
    }

    // when the turret has moved, stabilize all turrets on it
    int n = _turrets.Size(); Assert(vs._turretVisualStates.Size() == n);

    if (n > 0)
    {
      Matrix3 newTurretTrans = parentTrans * vs.TurretOrientation();
      for (int i=0; i<_turrets.Size(); i++)
      {
        TurretType *turretType = type._turrets[i];
        Turret *turret = _turrets[i];
        TurretVisualState *turretVisualState = vs._turretVisualStates[i];
        turret->Stabilize(*turretVisualState, obj, *turretType, oldTurretTrans, newTurretTrans);
      }
    }
  }

  // cloudlet sources
  if (obj->EnableVisualEffects(prec))
  {
    SimulateEffects(vs, obj, type, deltaT);
  }

  // Reflectors
  {
    // animate position
    _lights.AnimatePosition(_owner, type._lights);

    // switch on / off
    bool on = IsLightOn() 
#if _HELISIM_ROTORLIB
        && GetOwner()->IsElectricity()
#endif
    ;
    _lights.Switch(on, GetOwner(), type._lights);

    // Update the reflectors after changes
    _lights.UpdateAggLights(_owner);
  }
#if _ENABLE_HAND_IK
  if ( prec <= SimulateVisibleNear && _gunner)
  {
    PROFILE_SCOPE(pltIKsim);
    int level = 0;
    if (GWorld->GetCameraType() != CamExternal && GWorld->GetCameraType() != CamGroup)
    {
      if (GWorld->PlayerOn() == obj->Gunner()) level = obj->InternalLOD(ATGetInTurret);
      else if (GWorld->PlayerOn() == obj->Observer()) level = obj->InternalLOD(ATGetInTurret);
      else if (GWorld->PlayerOn() == obj->Driver()) level = obj->InternalLOD(ATGetInDriver);
      else level = obj->InternalLOD(ATGetInCargo);    
    }
    if (level != LOD_INVISIBLE && type._gunnerProxy[level].Present())
    {
      vs._IKIdent=false;
      obj->RecalcIKTransform(level, _gunner, type._gunnerProxy[level], type._gunnerLeftHandIndex, type._gunnerRightHandIndex,type._gunnerLeftLegIndex, type._gunnerRightLegIndex, vs._leftHandIK, vs._rightHandIK, vs._leftLegIK, vs._rightLegIK);    
    }
  }
#endif

  // simulate turrets
  int n = _turrets.Size(); Assert(vs._turretVisualStates.Size() == n);
  if (n > 0)
  {
    Matrix3 trans = parentTrans * vs.TurretOrientation();
    for (int i=0; i<n; i++)
    {
      TurretType *turretType = type._turrets[i];
      Turret *turret = _turrets[i];
      TurretVisualState *turretVisualState = vs._turretVisualStates[i];
      if (turret->Simulate(*turretVisualState, obj, *turretType, deltaT, prec, trans)) moved = true;
    }
  }
  return moved;
}

Matrix3 Turret::GetAimWantedWorld() const
{
  Assert(_gunStabilized);
  return Matrix3(MRotationY,_yRotWanted)*Matrix3(MRotationX,-_xRotWanted);
}

Matrix3 Turret::GetAimWantedRel() const
{
  Assert(!_gunStabilized);
  return Matrix3(MRotationY,_yRotWanted)*Matrix3(MRotationX,-_xRotWanted);
}

Matrix3 Turret::GetAimWantedWorld(Matrix3Par parent) const
{
  // convert to local space
  if (!_gunStabilized)
  {
    return parent*GetAimWantedRel();
  }
  else
  {
    return GetAimWantedWorld();
  }
}

void Turret::SetStabilization(bool stabilized, Matrix3Par parent)
{
  if (_gunStabilized==stabilized) return;
  _gunStabilized = stabilized;
  if (stabilized)
  {
    ToAbsolute(_xRotWanted,_yRotWanted,_xRotWanted,_yRotWanted,parent);
  }
  else
  {
    ToRelative(_xRotWanted,_yRotWanted,_xRotWanted,_yRotWanted,parent);
  }
}

#if _VBS3
bool Turret::AimWorld(TurretVisualState& vs, float azimuth, float elevation, bool forceSet)
{
  if (!_gunStabilized())
  {
    // if gun is not stabilized, it is broken an we cannot aim
    return false;
  }
  float yRotWanted=AngleDifference(-azimuth, _type->_neutralYRot);
  float xRotWanted=elevation-_type->_neutralXRot;

  if (_type->_maxTurn-_type->_minTurn<H_PI*2)
  {
    // if turning is limited, saturate around turning midpoint
    float midTurn = (_type->_maxTurn+_type->_minTurn)*0.5f;
    yRotWanted = AngleDifference(yRotWanted,midTurn)+midTurn;
  }
  else
  {
    yRotWanted=AngleDifference(yRotWanted,0);
  }

  _xRotWanted = xRotWanted;
  _yRotWanted = yRotWanted;

  if(forceSet)
  {
    vs._xRot = _xRotWanted;
    vs._yRot = _yRotWanted;
  }
  return true;
}
#endif

/**
TODO: use Turret::Aim in all vehicles - see Tank::AimWeapon
*/
bool Turret::AimWorld(TurretVisualState& vs, const TurretType &type, Vector3Val relDir)
{
  if (!_gunStabilized)
    // if gun is not stabilized, it is broken an we cannot aim
    return false;

  float yRotWanted = AngleDifference(-atan2(relDir.X(), relDir.Z()), type._neutralYRot);
  float xRotWanted = atan2(relDir.Y(), relDir.SizeXZ()) -type._neutralXRot;
  
  if (type._maxTurn-type._minTurn < H_PI*2)
  {
    // if turning is limited, saturate around turning midpoint
    float midTurn = (type._maxTurn + type._minTurn) * 0.5f;
    yRotWanted = AngleDifference(yRotWanted, midTurn) + midTurn;
  }
  else
    yRotWanted = AngleDifference(yRotWanted, 0);

  _xRotWanted = xRotWanted;
  _yRotWanted = yRotWanted;

#if 0
  if (_gunner == GWorld->PlayerOn())
  {
    DIAG_MESSAGE(100, Format("x %.2f->(%.2f), %.2f..%.2f", vs._xRot, _xRotWanted, type._minElev, type._maxElev));
    DIAG_MESSAGE(100, Format("y %.2f->(%.2f), %.2f..%.2f", vs._yRot, _yRotWanted, type._minTurn, type._maxTurn));
  }
#endif
  return true;
}

bool Turret::Aim(TurretVisualState& vs, const TurretType &type, Vector3Val relDir, Matrix3Par parent)
{
  
#if _VBS3 //scripted commander override
  if(_disableGunnerInput) return true;
#endif

  float yRotWanted=AngleDifference(-atan2(relDir.X(),relDir.Z()),type._neutralYRot);
  float xRotWanted=atan2(relDir.Y(),relDir.SizeXZ())-type._neutralXRot;
  
  if (type._maxTurn-type._minTurn<H_PI*2)
  {
    // if turning is limited, saturate around turning midpoint
    float midTurn = (type._maxTurn+type._minTurn)*0.5f;
    yRotWanted = AngleDifference(yRotWanted,midTurn)+midTurn;
  }
  else
  {
    yRotWanted=AngleDifference(yRotWanted,0);
  }

#if 0
  if (_gunner == GWorld->PlayerOn())
  {
    DIAG_MESSAGE(100, Format("x %.2f->%.2f, %.2f..%.2f", vs._xRot, xRotWanted, type._minElev, type._maxElev));
    DIAG_MESSAGE(100, Format("y %.2f->%.2f, %.2f..%.2f", vs._yRot, yRotWanted, type._minTurn, type._maxTurn));
  }
#endif


  Limit(xRotWanted,type._minElev,type._maxElev);
  Limit(yRotWanted,type._minTurn,type._maxTurn);

  float xToAim=fabs(xRotWanted-vs._xRot);
  float yToAim=fabs(yRotWanted-vs._yRot);

  if (_gunStabilized)
  {
    ToAbsolute(_xRotWanted,_yRotWanted,xRotWanted,yRotWanted,parent);
  }
  else
  {
    _xRotWanted = xRotWanted;
    _yRotWanted = yRotWanted;
  }

  if( xToAim+yToAim>1e-6 ) return true; // enable simulation
  return false;
}

void Turret::SetSpeed(TurretVisualState& vs, const TurretType &type, float speedX, float speedY, float deltaT, Matrix3Par parent)
{
#if _VBS3   //scripted commander override
  if(_disableGunnerInput) return;
#endif

  float xRotWanted =  0.15f * speedX;
  float yRotWanted =  0.15f * speedY;
  
  if (!_gunStabilized) return;
  const ViewPars* viewOptics = GetCurrentViewPars(&type);
  
  if(
    (viewOptics &&  viewOptics->_directionStabilized) &&
    (speedX == 0 && speedY == 0 &&  (!_lockOnGround || (fabs(_xSpeed)<0.01f && fabs(_ySpeed)<0.01f)))
    )
  { // when player is not aiming with mouse and turret is not moving because of momentum 
    // lock turret on Intersect With Ground Or Sea  position; used in UAV
    Matrix4Val shootTrans = _owner->GunTurretTransform(_owner->FutureVisualState(), type);
    Vector3Val dir =_owner->FutureVisualState().DirectionModelToWorld( Matrix3(MRotationY,vs._yRot) * Matrix3(MRotationX,-vs._xRot).Direction());
    Vector3 pos = _owner->FutureVisualState().PositionModelToWorld(type.GetTurretPos(shootTrans));

    if(_lockOnGround)
    {
      _lastLockedPosition = GLOB_LAND->IntersectWithGroundOrSea(pos, dir);
      _xSpeed = 0; _ySpeed = 0;
    }

    AimWorld(vs, type, _lastLockedPosition - pos);
    _lockOnGround = false;
  }
else
  {
#if _VBS3 //fast turrets had a bouncing effect sometimes
    float coef = 0.2;
#else
    float coef = 0.45;
#endif
    saturate(yRotWanted,-H_PI*coef,H_PI*coef);
    saturate(xRotWanted,-H_PI*coef,H_PI*coef);
    xRotWanted += vs._xRot;
    yRotWanted += vs._yRot;

    if (type._maxTurn-type._minTurn<H_PI*2)
    {
      // if turning is limited, saturate around turning midpoint
      float midTurn = (type._maxTurn+type._minTurn)*0.5f;
      yRotWanted = AngleDifference(yRotWanted,midTurn)+midTurn;
    }
    else
      yRotWanted=AngleDifference(yRotWanted,0);

    ToAbsolute(_xRotWanted,_yRotWanted,xRotWanted,yRotWanted,parent);
    _lockOnGround = true;
  }

#if 0
  if (_gunner == GWorld->PlayerOn())
  {
    TurretContext context;
    if (_owner->FindTurret(this, context) && context._turretType)
    {
      const TurretType &type = *context._turretType;
      DIAG_MESSAGE(100, Format("x %.2f->%.2f (absolute %.2f), %.2f..%.2f speedX=%.2f", vs._xRot, xRotWanted, _xRotWanted, type._minElev, type._maxElev, speedX));
      DIAG_MESSAGE(100, Format("y %.2f->%.2f (absolute %.2f), %.2f..%.2f speedY=%.2f", vs._yRot, yRotWanted, _yRotWanted, type._minTurn, type._maxTurn, speedY));
      DIAG_MESSAGE(100, Format("deltaX %.2f", vs._xRot-_xRotWanted));
      DIAG_MESSAGE(100, Format("deltaY %.2f", vs._yRot-_yRotWanted));

//      LogF("Turret::SetSpeed");
//      LogF("x %.2f->%.2f (absolute %.2f), %.2f..%.2f", vs._xRot, xRotWanted, _xRotWanted, type._minElev, type._maxElev);
//      LogF("y %.2f->%.2f (absolute %.2f), %.2f..%.2f", vs._yRot, yRotWanted, _yRotWanted, type._minTurn, type._maxTurn);
    }
  }
#endif
}

/**
@return true when turret has moved
\patch 1.12 Date 8/7/2001 by Ondra.
- Changed: Turret response to aiming quicker.
\patch 1.31 Date 11/20/2001 by Ondra.
- Changed: Turret over-aiming fixed.
*/

bool Turret::MoveWeapons(TurretVisualState& vs, const TurretType &type, AIBrain *unit, float deltaT, Matrix3Par parent)
{
  float xRotWanted = _xRotWanted;
  float yRotWanted = _yRotWanted;
  if (_gunStabilized)
  {
    ToRelative(xRotWanted,yRotWanted,xRotWanted,yRotWanted,parent);
    Limit(xRotWanted,type._minElev,type._maxElev);
    if (type._maxTurn-type._minTurn<H_PI*2)
    { // if turning is limited, saturate around turning midpoint
      float midTurn = (type._maxTurn+type._minTurn)*0.5f;
      yRotWanted = AngleDifference(yRotWanted,midTurn)+midTurn;
    }
    Limit(yRotWanted,type._minTurn,type._maxTurn);
  }

  bool ret = false;
  float speed, delta;
  float maxSpeed = 0;
  float ability = unit->GetAbility(AKAimingSpeed);


#if !_VBS3 //this doesn't make sense for us
  if (type.InGunnerMayFire(_gunner) || vs._crewHidden < 0.5)
#endif
  {
    // when gunner inside cannot fire, he also cannot handle gun (only turret)
    speed = (xRotWanted-vs._xRot)*4;
#if _VBS3_LASE_LEAD
    float laseDiff = _laseOffsetXWanted - _laseOffsetX;
    if(fabs(laseDiff) > 0.0001)
    {
      speed = laseDiff*4; //just use the lase value (freezes vertical input)
    }
#endif
    if (fabs(speed)>1e-2f) ret = true;
    saturateMax(maxSpeed,fabs(speed));
    delta=speed-_xSpeed;
#if _VBS3 //raise acceleration limit
    Limit(delta,-type._maxXRotSpeed*5.0f*deltaT,+type._maxXRotSpeed*5.0f*deltaT);
#else
    Limit(delta,-3*deltaT,+3*deltaT);
#endif
    _xSpeed+=delta;
    Limit(_xSpeed,-type._maxXRotSpeed*ability,type._maxXRotSpeed*ability);
    vs._xRot+=_xSpeed*deltaT;

#if _VBS3_LASE_LEAD
    if(fabs(laseDiff) > 0.0001)
    {
      _laseOffsetX += _xSpeed*deltaT;
    }
    else
      _laseOffsetX = _laseOffsetXWanted;
#endif
  }

  speed = AngleDifference(yRotWanted,vs._yRot)*4;
  if (fabs(speed)>1e-2f) ret = true;
  
  saturateMax(maxSpeed,fabs(speed));
  delta=speed-_ySpeed;
#if _VBS3 //raise acceleration limit
  Limit(delta,-type._maxYRotSpeed*5.0f*deltaT,+type._maxYRotSpeed*5.0f*deltaT);
#else
  Limit(delta,-3*deltaT,+3*deltaT);
#endif
  _ySpeed+=delta;
  Limit(_ySpeed,-type._maxYRotSpeed*ability,type._maxYRotSpeed*ability);
  vs._yRot+=_ySpeed*deltaT;
  if (type._maxTurn-type._minTurn<H_PI*2)
  {
    // if turning is limited, saturate around turning midpoint
    float midTurn = (type._maxTurn+type._minTurn)*0.5f;
    vs._yRot = AngleDifference(vs._yRot,midTurn)+midTurn;
  }
  else
  {
    vs._yRot=AngleDifference(vs._yRot,0);
  }
#if 0
  LogF("xSpeed %.2f", _xSpeed);
  LogF("ySpeed %.2f", _ySpeed);
#endif

  Limit(vs._xRot,type._minElev,type._maxElev);
  Limit(vs._yRot,type._minTurn,type._maxTurn);

  //_servoVol=floatMin(1,maxSpeed*4);
  float servoVolWanted=0;
  if( maxSpeed>0.01f ) servoVolWanted=1;
  delta=servoVolWanted-_servoVol;
  Limit(delta,-2*deltaT,+2*deltaT);
  _servoVol+=delta;

#if 0
  DIAG_MESSAGE(100, Format("_yRot = %.2f", vs._yRot));
  DIAG_MESSAGE(100, Format("_ySpeed = %.2f", _ySpeed));
#endif
  


  return ret;
}

Matrix4 TurretVisualState::TurretTransform(const EntityType *entType, const TurretType &type) const
{
  Vector3 yAxis = type.GetYAxis(entType);
  return Matrix4(MTranslation,yAxis) * Matrix4(MRotationY,_yRot) * Matrix4(MTranslation,-yAxis);
}

Matrix4 TurretVisualState::GunTransform(const EntityType *entType, const TurretType &type) const
{
  Vector3 xAxis = type.GetXAxis(entType);
  return Matrix4(MTranslation,xAxis) * Matrix4(MRotationX,-_xRot) * Matrix4(MTranslation,-xAxis);
}


TurretVisualState* TurretVisualState::GetTurret(const int *path, int size)
{
  if (size == 0) return this;
  int i = path[0];
  if (i < 0 || i >= _turretVisualStates.Size())
  {
    LogF("Invalid turret path");
    return NULL;
  }
  return _turretVisualStates[i]->GetTurret(path + 1, size - 1);
}


TurretVisualState const* TurretVisualState::GetTurret(const int *path, int size) const
{
  if (size == 0) return this;
  int i = path[0];
#if _VBS3
  if( i >= _turretVisualStates.Size() ) return NULL;
#endif
  return _turretVisualStates[i]->GetTurret(path + 1, size - 1);
}


void TurretVisualState::Interpolate(TurretVisualState const& t1state, float t, TurretVisualState& res) const
{
  TurretVisualState const& s = t1state;

  #define INTERPOLATE(x) res.x = x + t*(s.x - x)
  #define INTERPOLATE_ANGLE(x) res.x = x + t*AngleDifference(s.x, x)

  INTERPOLATE_ANGLE(_xRot);  // angle
  INTERPOLATE_ANGLE(_yRot);  // angle
  INTERPOLATE(_crewHidden);  // continuous

  #undef INTERPOLATE
  #undef INTERPOLATE_ANGLE

#define INTERPOLATE_MATRIX(x) res.x = x*(1-t) + s.x*t

  res._IKIdent = _IKIdent && s._IKIdent;
  INTERPOLATE_MATRIX(_leftLegIK);
  INTERPOLATE_MATRIX(_rightLegIK);

  // IK transforms
  INTERPOLATE_MATRIX(_leftHandIK);
  INTERPOLATE_MATRIX(_rightHandIK);

#undef INTERPOLATE

  int n = _turretVisualStates.Size();
//  res._turretVisualStates.Realloc(n);
//  res._turretVisualStates.Resize(n);
  for (int i=0; i<n; i++) {
    _turretVisualStates[i]->Interpolate(*s._turretVisualStates[i], t, *res._turretVisualStates[i]);
  }
}


Vector3 Turret::GetCenter(const EntityType *entType, const TurretType &type) const
{
  return type.GetTurretAimCenter(entType);
}

/**
move with xRot, yRot so that newDir is equal to oldDir
*/
void Turret::StabilizeXY(
  const TurretType &type,
  float &xRot, float &yRot, Matrix3Val oldTrans, Matrix3Val newTrans
)
{
  //need to stabilize after first simulation frame, so turret gets right direction
  //init() is too soon (vehicle is still heading north)
  if (fabsf(_stabilizeTo.DirectionUp().Size()) < 1e-4f)
    _stabilizeTo = oldTrans;

  Vector3Val oldDir = Matrix3(MRotationY,yRot) * Matrix3(MRotationX,-xRot).Direction();
  Matrix3 invTrans = newTrans.InverseRotation();

  // calculate what new direction would look like with no stabilization
  Vector3 gunDirNew = invTrans * (_stabilizeTo * oldDir);

  // calculate angle in the target coordinates      
  float yRotNew = -atan2(gunDirNew.X(),gunDirNew.Z());
  float xRotNew = atan2(gunDirNew.Y(),gunDirNew.SizeXZ());
  // adjust newDir to be equal to oldDir
  yRot += AngleDifference(yRotNew,yRot);
  xRot += AngleDifference(xRotNew,xRot);
  
  #if _DEBUG
    // recalculate new direction for verification
    Vector3Val newDir = Matrix3(MRotationY,yRot) * Matrix3(MRotationX,-xRot).Direction();
    Vector3 newCheck = newTrans * newDir;
    Vector3 oldCheck = oldTrans * oldDir;
    Assert(newCheck.Distance2(oldCheck)<1e-3);
  #endif
  
  Limit(xRot,type._minElev,type._maxElev);
  Limit(yRot,type._minTurn,type._maxTurn);
  
  _stabilizeTo = newTrans;
}

void Turret::ToRelative(float &xr, float &yr, float x, float y, Matrix3Par parent)
{
  // convert to vector
  Vector3Val dirRel = Matrix3(MRotationY,y)*Matrix3(MRotationX,-x).Direction();
  // convert vector from parent space
  Vector3Val dir = parent.InverseRotation()*dirRel;
  // convert back to angle
  yr = -atan2(dir.X(),dir.Z());
  xr = atan2(dir.Y(),dir.SizeXZ());
}
void Turret::ToAbsolute(float &x, float &y, float xr, float yr, Matrix3Par parent)
{
  // convert to vector
  Vector3Val dirRel = Matrix3(MRotationY,yr)*Matrix3(MRotationX,-xr).Direction();
  // convert vector to parent space
  Vector3Val dir = parent*dirRel;
  // convert back to angle
  y = -atan2(dir.X(),dir.Z());
  x = atan2(dir.Y(),dir.SizeXZ());
}

/*!
\patch 1.24 Date 9/24/2001 by Ondra.
- Fixed: Improved turret stabilization, especially in Multiplayer.
\patch_internal 1.24 Date 9/24/2001 by Ondra.
- Fixed: turret stabilization code is correct for even for bigger angles.
*/


void Turret::Stabilize(
  TurretVisualState& vs,
  const Entity *obj,
  const TurretType &type, Matrix3Val oldTrans, Matrix3Val newTrans
)
{
  // TurretOrientation is read before and after stabilizing this turret
  Matrix3 oldParent = oldTrans * vs.TurretOrientation();
  if (_gunStabilized && type._animBody >= 0)
  {
    // stabilize current x,y values
    float fnull= 0.0f;
    switch (type._stabilizedInAxes)
    {
      case SAxisY:StabilizeXY(type, fnull, vs._yRot, oldTrans, newTrans);break; // stabilize only Y axis
      case SAxisX:StabilizeXY(type, vs._xRot, fnull, oldTrans, newTrans);break; // stabilize only X axis
      case SAxisBoth:StabilizeXY(type, vs._xRot, vs._yRot, oldTrans, newTrans);break; // stabilize both axes
      // else no stabilization
    }
  }

  // _stabilizeTo need to be actualized even if _gunStabilized is disabled (e.g. gunner is turned out) 
  _stabilizeTo = newTrans;

  Matrix3 newParent = newTrans * vs.TurretOrientation();
  // keep turrets stabilized
  for (int i=0; i<_turrets.Size(); i++)
  {
    const TurretType *turretType = type._turrets[i];
    Turret *turret = _turrets[i];
    TurretVisualState *turretVisualState = vs._turretVisualStates[i];
    turret->Stabilize(*turretVisualState, obj, *turretType, oldParent, newParent);
  }

  // note: no need to stabilize wanted values - they are world space
}

void Turret::Stop(const TurretType &type, Matrix3Par parent)
{
  SetStabilization(false,parent);
  _servoVol=0;
  _xSpeed=0;
  _ySpeed=0;
}

void Turret::LockForward(const TurretType &type)
{
  _gunStabilized=false;
  _xRotWanted=type._neutralXRot;
  _yRotWanted=type._neutralYRot;
}

void Turret::GunBroken(const TurretType &type, Matrix3Par parent)
{
  SetStabilization(false,parent);
  _xRotWanted=type._minElev;
}

void Turret::TurretBroken(TurretVisualState& vs, const TurretType &type, Matrix3Par parent)
{
  SetStabilization(false, parent);
  _yRotWanted = vs._yRot; // no rotation

  // break turrets recursively
  Matrix3 trans = parent * vs.TurretOrientation();
  for (int i=0; i<_turrets.Size(); i++)
  {
    TurretType *turretType = type._turrets[i];
    Turret *turret = _turrets[i];
    TurretVisualState *turretVisualState = vs._turretVisualStates[i];
    turret->TurretBroken(*turretVisualState, *turretType, trans);
  }
}

Vector3 Turret::GetCurrentPosition() const
{
  return _owner ? _owner->GetCurrentPosition() : VZero;
}

void Turret::DestroyObject()
{
  _networkId = NetworkId::Null();
  _local = true;
}

bool Turret::SimulateHatch(TurretVisualState& vs, float deltaT, SimulationImportance prec)
{
  const float turnInOutSpeed = 1.0;
  if (_crewHiddenWanted > vs._crewHidden)
  {
    vs._crewHidden += turnInOutSpeed * deltaT;
    saturateMin(vs._crewHidden, _crewHiddenWanted);
    return true;
  }
  else if (_crewHiddenWanted < vs._crewHidden)
  {
    vs._crewHidden -= turnInOutSpeed * deltaT;
    saturateMax(vs._crewHidden, _crewHiddenWanted);
    return true;
  }
  return false;
}

RString Turret::GetDebugName() const
{
  return "Turret";
}

const char *Turret::GetVehicleName() const
{
  if (_owner)
  {
    return _owner->GetName();
  }
  return "unknown";
}

#if _VBS3_LASE_LEAD
void Turret::Lase(TurretVisualState& vs, Vector3& pos, Vector3& aimDir, Vector3 camDir)
{
  aimDir.Normalize();
  camDir.Normalize();

  _laseDistance = pos.Distance(GetCurrentPosition());    
  if(_laseDistance >= Glob.config.horizontZ)
    _laseDistance = -1;
  _laserWeaponAimed = _weaponsState.ValidatedCurrentWeapon();

  float offsetX = aimDir.Y() - camDir.Y(); //only vertical plane calculated!

  if (offsetX < 0 || _laseDistance < 100 || _laseDistance > 8000)
  {
    ClearFCS();
    return;
  }

  /* all in world coordinates
  if (_gunStabilized)
  {
  ToRelative(xRotWanted,yRotWanted,xRotWanted,yRotWanted,parent);
  }
  */
  // can't reach the required angle?
  if (vs._xRot + offsetX > _type->_maxElev)
    return;

  _laseOffsetXWanted = offsetX;
}
Turret::LLStatus Turret::GetLaseStatus()
{
  if(_laseOffsetX < 0 || _laseOffsetX == 0 || _weaponsState.ValidatedCurrentWeapon() != _laserWeaponAimed) return LLError;
  if(_laseOffsetX != _laseOffsetXWanted) return LLAdjusting;
  return LLReady;
}

void Turret::ClearFCS()
{
  _laseDistance = 0;
  _laseOffsetX = 0;
}
#endif //_VBS3_LASE_LEAD


DEFINE_NET_INDICES_EX(CreateTurret, NetworkObject, CREATE_TURRET_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateTurret, NetworkObject, UPDATE_TURRET_MSG, NoErrorInitialFunc)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionTurret, NetworkObject, UPDATE_POSITION_TURRET_MSG, NoErrorInitialFunc)

#define TURRET_MSG_LIST(XX) \
  XX(Create, CreateTurret) \
  XX(UpdateGeneric, UpdateTurret) \
  XX(UpdatePosition, UpdatePositionTurret)

DEFINE_NETWORK_OBJECT(Turret, NetworkObject, TURRET_MSG_LIST)

Turret *Turret::CreateObject(NetworkMessageContext &ctx)
{
  PREPARE_TRANSFER(CreateTurret)

  OLinkNO(Transport) owner;
  if (TRANSF_REF_BASE(vehicle, owner) != TMOK) return NULL;
  if (!owner) return NULL;

  TurretPath path;
  if (TRANSF_BASE(path, path) != TMOK) return NULL;

  Turret *turret = dynamic_cast<Turret *>(owner->GetTurret(path.Data(), path.Size()));
  if (!turret) return NULL;

  NetworkId objectId;
  if (TRANSF_BASE(objectCreator, objectId.creator) != TMOK) return NULL;
  if (TRANSF_BASE(objectId, objectId.id) != TMOK) return NULL;
  turret->SetNetworkId(objectId);
  turret->SetLocal(false);

  return turret;
}

NetworkMessageFormat &Turret::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCCreate:
    NetworkObject::CreateFormat(cls, format);
    CREATE_TURRET_MSG(CreateTurret, MSG_FORMAT)
    break;
  case NMCUpdateGeneric:
    NetworkObject::CreateFormat(cls, format);
    UPDATE_TURRET_MSG(UpdateTurret, MSG_FORMAT_ERR)
    break;
  case NMCUpdatePosition:
    NetworkObject::CreateFormat(cls, format);
    UPDATE_POSITION_TURRET_MSG(UpdatePositionTurret, MSG_FORMAT_ERR)
    break;
  default:
    NetworkObject::CreateFormat(cls, format);
    break;
  }
  return format;
}

bool IsInVehicle(Person *soldier, Transport *veh);
CameraType ValidateCamera(CameraType cam);

TMError Turret::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    TMCHECK(NetworkObject::TransferMsg(ctx))
    break;
  case NMCUpdateGeneric:
    TMCHECK(NetworkObject::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateTurret)
      if (ctx.IsSending())
      {
        TRANSF_REF(gunner)
      }
      else
      {
        Assert(_owner);

        // The following ToDelete check line fixes the PlayerDesync BUG reported here news:i9hkv0$6b7$1@new-server.localdomain
        if (_owner->ToDelete()) //ignore message when owner vehicle is marked to be deleted
          return TMOK;

        OLinkPerm<Person> gunner;
        TRANSF_REF_EX(gunner, gunner)

        // first check position changes
        if (gunner != _gunner && gunner && IsInVehicle(gunner, _owner))
        {
#if DIAG_CREW
          RString name;
          TurretContext context;
          if (_owner->FindTurret(this, context) && context._turretType) name = context._turretType->_gunnerName;
          LogF("### Turret '%s' update arrived - '%s' found in the vehicle", cc_cast(name), cc_cast(gunner->GetDebugName()));
#endif
          Ref<Action> action = new ActionTurret(ATMoveToTurret, _owner, this);
          _owner->ChangePosition(action, gunner);
        }

        if (gunner != _gunner)
        {
          TurretContext context;
          bool valid = _owner->FindTurret(this, context) && context._turretType;
#if DIAG_CREW
          RString name;
          if (valid) name = context._turretType->_gunnerName;
          LogF("### Turret '%s' update arrived - '%s' not found in the vehicle", cc_cast(name), cc_cast(gunner ? gunner->GetDebugName() : "NULL"));
#endif
          if (_gunner && valid)
          {
            // get out
#if DIAG_CREW
            LogF(" original gunner '%s' gets out", cc_cast(_gunner->GetDebugName()));
#endif
            AIBrain *unit = _gunner->Brain();
            if (unit)
            {
              bool isFocused = (GWorld->FocusOn() == unit);
              unit->DoGetOut(_owner, true, false);
              if (isFocused)
                GWorld->SwitchCameraTo(unit->GetVehicle(), ValidateCamera(GWorld->GetCameraType()), false);
            }
            else
            {
              // FIX - enable get out dead body (without brain)
              // calculate get out position
              GetInPoint pt = _owner->GetTurretGetOutPos(this, _gunner);
              Matrix4 transform;
              transform.SetPosition(pt._pos);
              transform.SetUpAndDirection(VUp, - pt._dir);
              // get out
              _owner->GetOutTurret(context, transform);
            }
          }
          if (gunner)
          {
            // get in
            _owner->GetInTurret(this, gunner, true, true);
            if (GWorld->GetRealPlayer() == gunner)
              GWorld->SwitchCameraTo(_owner, ValidateCamera(GWorld->GetCameraType()), false);
          }
        }
      }
      TRANSF(locked)
      TRANSF(crewHiddenWanted)
      TRANSF(searchLightOn)

      if (ctx.IsSending())
      {
        TRANSF_REFS(weapons)
        TRANSF_REFS(magazines)
      }
      else
      {
        // weapons
        // unassign old weapons
        for (int i=0; i<_weapons.Size(); i++)
        {
          Weapon *weapon = _weapons[i];
          if (weapon)
          {
            // switch owner of the weapon
            weapon->CancelHierarchyParent(_owner);
          }
        }
        // transfer weapons
        TRANSF_REFS(weapons)
        // assign current weapons
        for (int i=0; i<_weapons.Size(); i++)
        {
          Weapon *weapon = _weapons[i];
          if (weapon)
          {
            // switch owner of the weapon
            weapon->ItemMoveOut(_owner);
          }
        }

        // magazines
        // unassign old magazines
        for (int i=0; i<_magazines.Size(); i++)
        {
          Magazine *magazine = _magazines[i];
          if (magazine)
          {
            // switch owner of the magazine
            magazine->CancelHierarchyParent(_owner);
          }
        }
        // transfer magazines
        TRANSF_REFS(magazines)
        // assign current magazines
        for (int i=0; i<_magazines.Size(); i++)
        {
          Magazine *magazine = _magazines[i];
          if (magazine)
          {
            // switch owner of the magazine
            magazine->ItemMoveOut(_owner);
          }
        }
      }
      TRANSF_REF_EX(currentWeapon, _currentWeapon)

#if _VBS3 //Commander Override
      TRANSF_REF(overriddenByTurret)
#endif
    }
    break;
  case NMCUpdatePosition:
    TMCHECK(NetworkObject::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdatePositionTurret)
      TRANSF(gunStabilized)
      TRANSF(yRotWanted)
      TRANSF(xRotWanted)
    }
    break;
  default:
    TMCHECK(NetworkObject::TransferMsg(ctx))
    break;
  }

  return TMOK;
}

float Turret::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += NetworkObject::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateTurret)

      ICALCERR_NEQREF_PERM(Person, gunner, ERR_COEF_STRUCTURE)
      ICALCERR_NEQ(bool, locked, ERR_COEF_MODE)
      ICALCERR_NEQ(float, crewHiddenWanted, ERR_COEF_VALUE_MAJOR)

      ICALCERR_NEQ(bool, searchLightOn, ERR_COEF_VALUE_MAJOR)
#if _VBS3
      ICALCERR_NEQREF_PERM(Person, overriddenByTurret, ERR_COEF_STRUCTURE)
#endif

      // weapons
      int count = intMin(message->_weapons.Size(), _weapons.Size());
      int dif = (message->_weapons.Size() - count) + (_weapons.Size() - count);
      for (int i=0; i<count; i++)
      {
        if (_weapons[i])
        {
          if (message->_weapons[i] != _weapons[i]->GetNetworkId()) dif++;
        }
        else
        {
          if (!message->_weapons[i].IsNull()) dif++;
        }
      }
      error += ERR_COEF_MODE * dif;

      // magazines
      count = intMin(message->_magazines.Size(), _magazines.Size());
      dif = (message->_magazines.Size() - count) + (_magazines.Size() - count);
      for (int i=0; i<count; i++)
      {
        if (_magazines[i])
        {
          if (message->_magazines[i] != _magazines[i]->GetNetworkId()) dif++;
        }
        else
        {
          if (!message->_magazines[i].IsNull()) dif++;
        }
      }
      error += ERR_COEF_MODE * dif;

      ICALCERRE_NEQREF_SOFT(Weapon, currentWeapon, _currentWeapon, ERR_COEF_MODE)
    }
    break;
  case NMCUpdatePosition:
    error += NetworkObject::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdatePositionTurret)

      ICALCERR_NEQ(bool, gunStabilized, ERR_COEF_MODE)
      ICALCERR_ABSDIF(float, yRotWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, xRotWanted, ERR_COEF_VALUE_MAJOR)
    }
    break;
  default:
    error += NetworkObject::CalculateError(ctx);
    break;
  }
  return error;
}

void Turret::CreateSubobjects(Transport *owner, TurretPath &path)
{
  GetNetworkManager().CreateSubobject(owner, path, this);

  // weapons
  for (int i=0; i<_weapons.Size(); i++)
  {
    Weapon *weapon = _weapons[i];
    if (weapon) GetNetworkManager().CreateVehicle(weapon, VLTOut, "", -1);
  }
  // magazines
  for (int i=0; i<_magazines.Size(); i++)
  {
    Magazine *magazine = _magazines[i];
    if (magazine) GetNetworkManager().CreateVehicle(magazine, VLTOut, "", -1);
  }

  int n = path.Size();
  for (int i=0; i<_turrets.Size(); i++)
  {
    path.Resize(n + 1);
    path[n] = i;
    _turrets[i]->CreateSubobjects(owner, path);
  }
}


#if _VBS2 // fire control system
// not yet implemented, code is required as part of upcoming VBS2 fire control system
// specialist helper method for solving time of flight equations
static void SolveExpansionFull(float target, float friction, float frameRate, float result[]) 
{
  float limit = 1.0/friction;
  // can solve - within "range"
  if (limit>=target) 
  {
  //  float result[3];
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
    result[0] = (count+fraction)/frameRate;
    result[1] = series;
    result[2] = accSeries;
  }
  else 
  {
    result[0] = -1;
  }
}

// finds a weapon's angle of elevation so that it can hit a target a specified horizontal and vertical distance away  
void static FindElevation(float xdist, float ydist, float muzzleVelocity, float friction, float frameRate, float solution[]) 
{
  float soln;
  float solnHeight;
  float lowerAlpha;
  float upperAlpha;
  float lowerHeight;
  float upperHeight;
  float powerSoln[3];
  float acceleration = -9.8;

  solution[1] = -1;   // in case we can't solve
  lowerAlpha = -45.0*H_PI/180;
  upperAlpha = 45.0*H_PI/180;
  SolveExpansionFull(frameRate*xdist/(cos(lowerAlpha)*muzzleVelocity),friction,frameRate,powerSoln);
  if (powerSoln[0]==-1) 
  {
    lowerAlpha = 0.0;
    SolveExpansionFull(frameRate*xdist/(cos(lowerAlpha)*muzzleVelocity),friction,frameRate,powerSoln);
  }
  if (powerSoln[0]==-1)
    return;
  lowerHeight = muzzleVelocity*sin(lowerAlpha)*powerSoln[1]/frameRate + acceleration/frameRate/frameRate*powerSoln[2];
  SolveExpansionFull(frameRate*xdist/(cos(lowerAlpha)*muzzleVelocity),friction,frameRate,powerSoln);
  if (powerSoln[0]==-1)
    return;
  upperHeight = muzzleVelocity*sin(lowerAlpha)*powerSoln[1]/frameRate + acceleration/frameRate/frameRate*powerSoln[2];

  int cnt = 0;

  #define EPSILON 0.000001
  while (fabs(lowerAlpha-upperAlpha)>EPSILON) 
  {
    soln = (lowerAlpha+upperAlpha)/2.0;
    SolveExpansionFull(frameRate*xdist/(cos(soln)*muzzleVelocity),friction,frameRate,powerSoln);
    solnHeight = muzzleVelocity*sin(soln)*powerSoln[1]/frameRate + acceleration/frameRate/frameRate*powerSoln[2];
    if (solnHeight<ydist) 
    {
      lowerAlpha = soln;
      lowerHeight = solnHeight;
    }
    else 
    {
      upperAlpha = soln;
      upperHeight = solnHeight;
    }
    cnt++;
  }
  solution[0] = lowerAlpha;
  solution[1] = powerSoln[0];
}
#endif
