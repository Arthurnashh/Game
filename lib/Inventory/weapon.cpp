#include "../wpch.hpp"
#include "magazine.hpp"
#include "weapon.hpp"
#include "itemoptics.hpp"
#include "../fileLocator.hpp"
#include "../motion.hpp"
#include "../Network/network.hpp"
#include <Es/ErrorProp/errorProp.hpp>
#include "../AI/ai.hpp"
#include "../person.hpp"

// class MuzzleType
MuzzleType::MuzzleType()
{
}

MuzzleType::~MuzzleType()
{
}

void MuzzleType::Init(ParamEntryPar cls, const WeaponType *weapon)
{
  _parClass = cls.GetClassInterface();
  OpticsInfo opticInfo;
  bool multipleOptics = cls.FindEntry("OpticsModes");
 
  _displayName = cls >> "displayName";
  _magazineReloadTime = cls >> "magazineReloadTime";
  GetValue(_sound, cls >> "drySound");
  _soundContinuous = cls>>"soundContinuous";
  GetValue(_reloadSound, cls >> "reloadSound");
  GetValue(_reloadMagazineSound, cls >> "reloadMagazineSound");
  _soundDuration = _reloadSoundDuration = _reloadMagazineSoundDuration = 0;
  _bullets.Load(cls, "soundBullet");

  if (_reloadSound.name.GetLength() > 0)
    _reloadSoundDuration = GSoundsys ? GSoundsys->GetWaveDuration(_reloadSound.name) : 0;
  if (_reloadMagazineSound.name.GetLength() > 0)
    _reloadMagazineSoundDuration = GSoundsys ? GSoundsys->GetWaveDuration(_reloadMagazineSound.name) : 0;
  if (cls.CheckIfEntryExists("reloadAction")) 
    _reloadAction = cls>>"reloadAction";

  if (_sound.name.GetLength() > 0)
    _soundDuration = GSoundsys ? GSoundsys->GetWaveDuration(_sound.name) : 0;
  
  _aiDispersionCoefX = cls >> "aiDispersionCoefX";
  _aiDispersionCoefY = cls >> "aiDispersionCoefY";

  _canBeLocked = cls >> "canLock";
  _ballisticsComputer = cls >> "ballisticsComputer";
  _enableAttack = cls >> "enableAttack";
  _optics = cls >> "optics";
  _primary = cls >> "primary";
  _showSwitchAction = cls >> "showSwitchAction";
  if (_primary == 0)
    _showSwitchAction = true;
  _showEmpty = cls >> "showEmpty";
  _autoReload = cls >> "autoReload";
  _backgroundReload = cls >> "backgroundReload";
  _canAutoAim = cls >> "autoAimEnabled";
  _showAimCursorInternal = cls >> "showAimCursorInternal";
  //_ffMag = cls >> "ffMagnitude";
  //_ffFreq = cls >> "ffFrequency";
  _irDistance = cls >> "irDistance";

  _shotSpreadAngle = cls >> "fireSpreadAngle";

  if(!multipleOptics)
  {
    opticInfo._opticsZoomMin = cls >> "opticsZoomMin";
    opticInfo._opticsZoomMax = cls >> "opticsZoomMax";
    opticInfo._opticsZoomInit = cls >> "opticsZoomInit";
    opticInfo._distanceZoomMin = cls >> "distanceZoomMin";
    opticInfo._distanceZoomMax = cls >> "distanceZoomMax";

    if(cls.FindEntry("opticsDisplayName")) 
      opticInfo._opticsDisplayName = cls >> "opticsDisplayName";

    opticInfo._opticsDisablePeripherialVision = cls >> "opticsDisablePeripherialVision";
    opticInfo._opticsFlare = cls>>"opticsFlare";
    opticInfo._id = cls >> "opticsID";

    if(cls.FindEntry("discretefov"))
    {
      ParamEntryVal discreteFov = cls >> "discretefov";
      if(discreteFov.GetSize() > 0)
      {
        opticInfo._discreteFov.Realloc(discreteFov.GetSize());
        opticInfo._discreteFov.Resize(discreteFov.GetSize());
        for (int i=0; i<discreteFov.GetSize(); i++)
          opticInfo._discreteFov[i] = discreteFov[i];
      }
      opticInfo._discreteInitIndex = cls >> "discreteInitIndex";
    }
    if(cls.FindEntry("discreteDistance"))
    {
      ParamEntryVal discreteDist = cls >> "discreteDistance";
      if(discreteDist.GetSize() > 0)
      {
        opticInfo._discreteDistance.Realloc(discreteDist.GetSize());
        opticInfo._discreteDistance.Resize(discreteDist.GetSize());
        for (int i=0; i<discreteDist.GetSize(); i++)
          opticInfo._discreteDistance[i] = discreteDist[i];
      }
      opticInfo._discreteDistanceInitIndex = cls >> "discreteDistanceInitIndex";
    }
  }

  _forceOptics = (cls>>"forceOptics").GetInt();
  _useAsBinocular = cls>>"useAsBinocular";

#if _VBS3
  //weapons also use viewoptics
  if(cls.FindEntry("ViewOptics"))
  {
    _viewOptics.Load(cls >> "ViewOptics");
    _opticsZoomMin = _viewOptics._minFov;
    _opticsZoomMax = _viewOptics._maxFov;
    _opticsZoomInit = _viewOptics._initFov;
  }
#endif

  _muzzlePos = VZero;
  _muzzleEnd = VZero;
  _muzzleDir = VForward;
  //_cameraPos = VZero;
  //_cameraDir = VForward;
  _irLaserPos = VZero;
  _irLaserDir = VForward;
#if _VBS3
  _laserPos = VZero;
#endif

  _cartridgeOutPosIndex = -1;
  _cartridgeOutEndIndex = -1;

  int n = (cls >> "magazines").GetSize();
  _magazines.Resize(n);
  for (int i=0; i<n; i++)
  {
    RStringB magazine = (cls >> "magazines")[i];
    _magazines[i] = dynamic_cast<MagazineType *>(VehicleTypes.New(magazine));
  }
  if (_magazines.Size() > 0)
    _typicalMagazine = _magazines[0];

  if (cls.FindEntry("magazineSlot")) _magazineSlot = cls >> "magazineSlot";
  if (_magazineSlot.IsEmpty()) _magazineSlot = RString("magazine") + cls.GetName();

  ParamEntryPtr entry = (Pars >> "CfgInventorySlots").FindEntry(_magazineSlot);
  if (entry)
  {
    RString FindPicture(RString name);
    _magazineSlotIcon  = GlobLoadTexture(FindPicture((*entry) >> "icon"));
  }

  n = (cls >> "modes").GetSize();
  _modes.Resize(n);
  for (int i=0; i<n; i++)
  {
    _modes[i] = new WeaponModeType();
    RStringB mode = (cls >> "modes")[i];
    if (stricmp(mode, "this") == 0)
      _modes[i]->Init(cls);
    else
      _modes[i]->Init(cls >> mode);
  }

#if _VBS3 // MuzzleEvent
  ConstParamEntryPtr array = cls.FindEntry("EventHandlers");
  if (array)
  {
    for (int i=0; i<NMuzzleEvent; i++)
    {
      MuzzleEvent e = (MuzzleEvent)i;
      RString name = FindEnumName(e);
      ConstParamEntryPtr entry = array->FindEntry(name);
      if (entry)
      {
        RStringB expression = *entry;
        _eventHandlers[e] = expression;
      }
    }
  }
  _showTrajectory = cls.ReadValue("showtrajectory", false);
#endif

  entry = cls.FindEntry("flashLight");
  if (entry)
  {
    _gunLightInfo = new FlashLigthInfo;
    
    if (!_gunLightInfo->Load(*entry.GetPointer()))
    {
      _gunLightInfo.Free();
    }
  }

  if(!multipleOptics)
  {
    ParamEntryVal cfgOpticsPars = Pars >> "CfgOpticsEffect";
    ParamEntryVal ppeEntry = cls >> "opticsPPEffects";
    if (ppeEntry.IsArray())
    {
      int size = ppeEntry.GetSize();
      opticInfo._ppEffectType.Realloc(size);
      opticInfo._ppEffectType.Resize(size);

      for (int i = 0; i < size; i++)
      {
        // effect type
        RString name = ppeEntry[i];
        if (cfgOpticsPars.FindEntry(name))
        {
          ParamEntryVal eEntry = cfgOpticsPars >> name;
          if (eEntry.IsClass())
            opticInfo._ppEffectType[i].Load(eEntry);
        }
      }
    }


    if (cls.FindEntry("visionMode"))
    {
      ParamEntryVal entry = cls >> "visionMode";
      bool tiWanted = false;

      if (entry.IsArray())
      {
        int count = entry.GetSize();
        opticInfo._visionModes.Realloc(count);
        opticInfo._visionModes.Resize(count);

        for (int i = 0; i < count; i++)
        {
          const IParamArrayValue &arVal = entry[i];

          RString effectName = arVal;
          VisionMode vMode = GetEnumValue<VisionMode>(cc_cast(effectName));

          if (vMode < 0 || vMode >= VMCount) vMode = VMNormal;

          opticInfo._visionModes[i] = vMode;
          if (vMode == VMTi) tiWanted = true;
        }

        if (tiWanted)
        {
          if (cls.FindEntry("thermalMode"))
          {
            entry = cls >> "thermalMode";

            int count = entry.GetSize();
            opticInfo._thermalModes.Realloc(count);
            opticInfo._thermalModes.Resize(count);

            for (int i = 0; i < count; i++)
            {
              const IParamArrayValue &arVal = entry[i];
              opticInfo._thermalModes[i] = arVal;
            }
          }
          else
          {
            RptF(Format("Warning: Thermal vision mode defined, but modes not. WeaponType: %s", 
              weapon ? cc_cast(weapon->GetName()) : ""));
          }
        }
      }
    }
    _opticsInfo.Add(opticInfo);
  }

  InitOpticModes(cls, weapon, _opticsInfo);

  _opticsInfo.Compact();

  _hasTiVision = false;

  // look for Ti vision
  for (int i = 0; i < _opticsInfo.Size(); i++)
  {
    if (_opticsInfo[i].HasVisionMode(VMTi))
    {
      _hasTiVision = true;
      break;
    }
  }
}

bool MuzzleType::InitOpticModes(ParamEntryPar cls, const WeaponType *weapon, AutoArray<OpticsInfo> &out)
{
  if(cls.FindEntry("OpticsModes"))
  {
    ParamEntryVal entry = cls >> "OpticsModes";
    for(int i=0; i<entry.GetEntryCount(); i++)
    {
      OpticsInfo opticInfo;
      ParamEntryVal param = entry.GetEntry(i);
      opticInfo._opticsZoomMin = param >> "opticsZoomMin";
      opticInfo._opticsZoomMax = param >> "opticsZoomMax";
      opticInfo._opticsZoomInit = param >> "opticsZoomInit";
      opticInfo._distanceZoomMin = param >> "distanceZoomMin";
      opticInfo._distanceZoomMax = param >> "distanceZoomMax";

      if(cls.FindEntry("opticsDisplayName")) 
        opticInfo._opticsDisplayName = cls >> "opticsDisplayName";

      opticInfo._opticsFlare = param >> "opticsFlare";
      opticInfo._opticsDisablePeripherialVision = param >> "opticsDisablePeripherialVision";
      opticInfo._id = param >> "opticsID";

      if(param.FindEntry("discretefov"))
      {
        ParamEntryVal discreteFov = param >> "discretefov";
        if(discreteFov.GetSize() > 0)
        {
          opticInfo._discreteFov.Realloc(discreteFov.GetSize());
          opticInfo._discreteFov.Resize(discreteFov.GetSize());
          for (int i=0; i<discreteFov.GetSize(); i++)
          {
            opticInfo._discreteFov[i] = discreteFov[i];

          }
        }
        opticInfo._discreteInitIndex = param >> "discreteInitIndex";
      }
      if(param.FindEntry("discreteDistance"))
      {
        ParamEntryVal discreteDist = param >> "discreteDistance";
        if(discreteDist.GetSize() > 0)
        {
          opticInfo._discreteDistance.Realloc(discreteDist.GetSize());
          opticInfo._discreteDistance.Resize(discreteDist.GetSize());
          for (int i=0; i<discreteDist.GetSize(); i++)
            opticInfo._discreteDistance[i] = discreteDist[i];
        }
        opticInfo._discreteDistanceInitIndex = param >> "discreteDistanceInitIndex";
      }

      ParamEntryVal cfgOpticsPars = Pars >> "CfgOpticsEffect";
      ParamEntryVal ppeEntry = param >> "opticsPPEffects";
      if (ppeEntry.IsArray())
      {
        int size = ppeEntry.GetSize();
        opticInfo._ppEffectType.Realloc(size);
        opticInfo._ppEffectType.Resize(size);

        for (int j = 0; j < size; j++)
        {
          // effect type
          RString name = ppeEntry[j];
          if (cfgOpticsPars.FindEntry(name))
          {
            ParamEntryVal eEntry = cfgOpticsPars >> name;
            if (eEntry.IsClass())
              opticInfo._ppEffectType[j].Load(eEntry);
          }
        }
      }

      if (param.FindEntry("visionMode"))
      {
        ParamEntryVal entry = param >> "visionMode";
        bool tiWanted = false;

        if (entry.IsArray())
        {
          int count = entry.GetSize();
          opticInfo._visionModes.Realloc(count);
          opticInfo._visionModes.Resize(count);

          for (int j = 0; j < count; j++)
          {
            const IParamArrayValue &arVal = entry[j];

            RString effectName = arVal;
            VisionMode vMode = GetEnumValue<VisionMode>(cc_cast(effectName));

            if (vMode < 0 || vMode >= VMCount) vMode = VMNormal;

            opticInfo._visionModes[j] = vMode;
            if (vMode == VMTi) tiWanted = true;
          }

          if (tiWanted)
          {
            if (param.FindEntry("thermalMode"))
            {
              entry = param >> "thermalMode";

              int count = entry.GetSize();
              opticInfo._thermalModes.Realloc(count);
              opticInfo._thermalModes.Resize(count);

              for (int j = 0; j < count; j++)
              {
                const IParamArrayValue &arVal = entry[j];
                opticInfo._thermalModes[j] = arVal;
              }
            }
            else
            {
              RptF(Format("Warning: Thermal vision mode defined, but modes not. WeaponType: %s", 
                weapon ? cc_cast(weapon->GetName()) : ""));
            }
          }
        }
      }

      out.Add(opticInfo);
    }
    return true;
  }
  return false;
}

#if _VBS3 //MuzzleEvent

DEFINE_ENUM(MuzzleEvent,ME,MUZZLE_EVENT_ENUM)

void MuzzleType::OnEvent(MuzzleEvent event, const GameValue &pars)
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

bool MuzzleType::IsEventHandler(MuzzleEvent event) const
{
  RString handler = _eventHandlers[event];
  if (handler.GetLength()>0) return true;
  return false;
}
#endif

void MuzzleType::NextVisionMode(VisionMode& mode, int &tiModeIndex, int opticsMode ) const
{
  _opticsInfo[opticsMode].NextVisionMode(mode, tiModeIndex);
}

void MuzzleType::GetInitVisionMode(VisionMode& mode, int &tiModeIndex, int opticsMode) const
{
  _opticsInfo[opticsMode].GetInitVisionMode(mode, tiModeIndex);
}

static const EnumName VisionModeNames[]=
{
  EnumName(VMNormal, "Normal"),
  EnumName(VMNvg, "NVG"),
  EnumName(VMTi, "Ti"),
  EnumName()
};

template<>
const EnumName *GetEnumNames(VisionMode dummy)
{
  return VisionModeNames;
}

static void InitCursorTexture(CursorTextureInfo &info, RString name)
{
  ConstParamEntryPtr entry = (Pars >> "CfgWeaponCursors").FindEntry(name);
  if (!entry)
  {
    info.texture = GlobLoadTexture(GetPictureName(name));
    info.fade = 1.0f;
    info.sections.Clear();
    return;
  }

  RStringB cursorName = (*entry) >> "texture";
  info.texture = GlobLoadTexture(GetPictureName(cursorName));
  info.fade = (*entry) >> "fade";
  ConstParamEntryPtr val = entry->FindEntry("color");
  if (val) info.color = GetPackedColor(*val);
  val = entry->FindEntry("shadowEnabled");
  if (val) info.shadow = *val;

  ParamEntryVal sections = (*entry) >> "Sections";
  int n = sections.GetEntryCount();
  info.sections.Realloc(n);
  info.sections.Resize(n);
  for (int i=0; i<n; i++)
  {
    ParamEntryVal src = sections.GetEntry(i);
    CursorTextureSection &dst = info.sections[i];
    dst.uMin = src >> "uMin";
    dst.vMin = src >> "vMin";
    dst.uMax = src >> "uMax";
    dst.vMax = src >> "vMax";
    dst.xOffset = src >> "xOffset";
    dst.yOffset = src >> "yOffset";
  }
}

/*!
\patch 5153 Date 4/12/2007 by Jirka
- New: Weapons - optics camera can have a different direction than muzzle direction now
*/

void MuzzleType::InitShape(const WeaponType *weapon)
{
  const ParamEntry &cls = *_parClass;

  bool multipleOptics = cls.FindEntry("OpticsModes");
  _opticsEnable = false;
  
  if(!multipleOptics && _opticsInfo.Size()>0)
  {
    if(cls.FindEntry("useModelOptics")) _opticsInfo[0]._useModelOptics = cls>>"useModelOptics";
    else _opticsInfo[0]._useModelOptics = true;
  }

  RStringB oModelName = cls>>"modelOptics";
#if _VBS3 //opticsmodel supported in viewOptics class
  if(_viewOptics._opticsModel.GetLength() > 0) oModelName = _viewOptics._opticsModel;
#endif

  if (oModelName.GetLength()>0)
  {
    static RStringB noModel("-");
    if (oModelName!=noModel)
    {
      _opticsModel = Shapes.New(GetShapeName(oModelName),true,false);
      _opticsEnable = true;
    }
    else
      _opticsEnable = true;
  }
  if (_opticsModel)
    _opticsModel->AddLoadHandler(this);

  RStringB cursorName = cls >> "cursor";
  if (cursorName.GetLength() > 0)
    InitCursorTexture(_cursorTexture, cursorName);

  cursorName = cls >> "cursorAim";
  if (cursorName.GetLength()>0)
    InitCursorTexture(_cursorAimTexture, cursorName);

  _cursorSize = cls>>"cursorSize";
  
  _muzzlePos = VZero;
  _muzzleEnd = VZero;
  _muzzleDir = VForward;
#if _VBS3
  _laserPos = VZero;
#endif

  _cartridgeOutPosIndex = -1;
  _cartridgeOutEndIndex = -1;

  LODShape *shape = weapon->GetShape();

  if (shape)
  {
    // get weapon positions
    const Shape *mem = shape->MemoryLevel();
    if (mem)
    {
      {
        RString pos = cls >> "muzzlePos";
        RString end = cls >> "muzzleEnd";
        Vector3Val vPos = shape->NamedPoint(shape->FindMemoryLevel(),pos);
        Vector3Val vEnd = shape->NamedPoint(shape->FindMemoryLevel(),end);
#if _VBS3
        if(cls.FindEntry("laserPos"))
        {
          RString lpPos = cls >> "laserPos";
          _laserPos = shape->NamedPoint(shape->FindMemoryLevel(),lpPos);
        }
#endif
        
        _muzzlePos = vPos;
        _muzzleEnd = vEnd;
        _muzzleDir = (vPos - vEnd).Normalized();
        if (_muzzleDir.SquareSize()<Square(0.1))
        {
          if (pos.GetLength()>0 && end.GetLength()>0)
            RptF("Bad muzzle direction in %s:'%s'-'%s'",cc_cast(GetName()),cc_cast(pos),cc_cast(end));
          _muzzleDir = VForward;
        }

        ConstParamEntryPtr entryIrPos = cls.FindEntry("irLaserPos");
        ConstParamEntryPtr entryIrEnd = cls.FindEntry("irLaserEnd");

        if (entryIrPos && entryIrEnd)
        {
          RString posIr = *entryIrPos;
          RString endIr = *entryIrEnd;

          if (shape->FindNamedSel(mem, posIr) >= 0 && shape->FindNamedSel(mem, endIr) >= 0)
          {
            Vector3Val vPos = shape->NamedPoint(shape->FindMemoryLevel(),posIr);
            Vector3Val vEnd = shape->NamedPoint(shape->FindMemoryLevel(),endIr);

            _irLaserPos = vPos;
            _irLaserDir = (vPos - vEnd).Normalized();

            if (_irLaserDir.SquareSize()<Square(0.1))
            {
              if (pos.GetLength()>0 && end.GetLength()>0)
                RptF("Bad ir laser direction in %s:'%s'-'%s'",cc_cast(GetName()),cc_cast(pos),cc_cast(end));
              _irLaserDir = VForward;
            }
          }
        }

        if(!multipleOptics && _opticsInfo.Size()>0)
        {
          InitCameraParams(cls, shape, _opticsInfo[0]);
        }
        InitCameraParams(cls, shape, _opticsCameraInfo);
      }
      {
        RString pos = cls >> "cartridgePos";
        RString end = cls >> "cartridgeVel";

        // point indices
        _cartridgeOutPosIndex = shape->PointIndex(mem,pos);
        _cartridgeOutEndIndex = shape->PointIndex(mem,end);

        _cartridgeOutPos = shape->NamedPoint(shape->FindMemoryLevel(),pos);
        Vector3 vEnd = shape->NamedPoint(shape->FindMemoryLevel(),end);
        _cartridgeOutVel = (vEnd - _cartridgeOutPos)*50;
      }

      if (_gunLightInfo)
      {
        _gunLightInfo->Init(shape);
      }
    }
  }

  InitShapeOpticsShape(weapon);
}

void MuzzleType::InitCameraParams(const ParamEntry &cls, LODShape *shape, OpticsCameraInfo &opticsInfo) const
{
  RString nameCameraPos = cls >> "memoryPointCamera";
  const Shape *mem = shape->MemoryLevel();
  if (shape->FindNamedSel(mem,nameCameraPos) >= 0)
    opticsInfo._cameraPos = shape->NamedPoint(shape->FindMemoryLevel(),nameCameraPos);
  else
  {
    // temporary work-around - camera position not in model
    //LogF("Camera position missing in weapon %s",cc_cast(weapon->_model->GetName()));
    opticsInfo._cameraPos = Vector3(0.193,0.166,0) - shape->BoundingCenter();
  }
 
  opticsInfo._cameraDir = _muzzleDir;
 
  ConstParamEntryPtr entry = cls.FindEntry("cameraDir");
  if (entry)
  {
    RString nameCameraDir = *entry;
    if (shape->FindNamedSel(mem, nameCameraDir) >= 0)
    {
      opticsInfo._cameraDir = opticsInfo._cameraPos - shape->NamedPoint(shape->FindMemoryLevel(), nameCameraDir);
      opticsInfo._cameraDir.Normalize();
      if (opticsInfo._cameraDir.SquareSize() < Square(0.1))
      {
        RptF("Bad camera direction in %s:'%s'-'%s'", cc_cast(GetName()), cc_cast(nameCameraPos), cc_cast(nameCameraDir));
        opticsInfo._cameraDir = _muzzleDir;
      }
    }
  }
  opticsInfo._neutralXRot = atan2(opticsInfo._cameraDir.Y(), opticsInfo._cameraDir.SizeXZ());
  opticsInfo._neutralYRot = 0;
}

void MuzzleType::InitShapeOpticsShape(const WeaponType *weapon)
{
  const ParamEntry &cls = *_parClass;

  RString pos = cls >> "muzzlePos";
  RString end = cls >> "muzzleEnd";

  if(cls.FindEntry("OpticsModes"))
  {
    ParamEntryVal entry = cls >> "OpticsModes";
    for(int i=0; i<entry.GetEntryCount(); i++)
    {
      if(i >= _opticsInfo.Size()) break;
      OpticsInfo *opticInfo = &_opticsInfo[i];
      ParamEntryVal param = entry.GetEntry(i);

      opticInfo->_useModelOptics = param>>"useModelOptics";

      LODShape *shape = weapon->GetShape();
      if (shape)
      {
        // get weapon positions
        const Shape *mem = shape->MemoryLevel();
        if (mem)
        {
          {
            Vector3Val vPos = shape->NamedPoint(shape->FindMemoryLevel(),pos);
            Vector3Val vEnd = shape->NamedPoint(shape->FindMemoryLevel(),end);

            _muzzlePos = vPos;
            _muzzleEnd = vEnd;
            _muzzleDir = (vPos - vEnd).Normalized();
            if (_muzzleDir.SquareSize()<Square(0.1))
            {
              if (pos.GetLength()>0 && end.GetLength()>0)
                RptF("Bad muzzle direction in %s:'%s'-'%s'",cc_cast(GetName()),cc_cast(pos),cc_cast(end));
              _muzzleDir = VForward;
            }

            {
              RString nameCameraPos = param >> "memoryPointCamera";
              if (shape->FindNamedSel(mem,nameCameraPos) >= 0)
                opticInfo->_cameraPos = shape->NamedPoint(shape->FindMemoryLevel(),nameCameraPos);
              else
              {
                // temporary work-around - camera position not in model
                //LogF("Camera position missing in weapon %s",cc_cast(weapon->_model->GetName()));
                opticInfo->_cameraPos = Vector3(0.193,0.166,0) - shape->BoundingCenter();
              }

              opticInfo->_cameraDir = _muzzleDir;

              ConstParamEntryPtr entry = param.FindEntry("cameraDir");
              if (entry)
              {
                RString nameCameraDir = *entry;
                if (shape->FindNamedSel(mem, nameCameraDir) >= 0)
                {
                  opticInfo->_cameraDir = opticInfo->_cameraPos - shape->NamedPoint(shape->FindMemoryLevel(), nameCameraDir);
                  opticInfo->_cameraDir.Normalize();
                  if (opticInfo->_cameraDir.SquareSize() < Square(0.1))
                  {
                    RptF("Bad camera direction in %s:'%s'-'%s'", cc_cast(GetName()), cc_cast(nameCameraPos), cc_cast(nameCameraDir));
                    opticInfo->_cameraDir = _muzzleDir;
                  }
                }
              }
              opticInfo->_neutralXRot = atan2(opticInfo->_cameraDir.Y(), opticInfo->_cameraDir.SizeXZ());
              // TODO: 
              opticInfo->_neutralYRot = 0;
            }
          }
        }
      }
    }
  }
}

void MuzzleType::LODShapeLoaded(LODShape *shape)
{
  if (shape==_opticsModel)
  {
    if (shape->NLevels() > 0)
    {
      shape->Level(0)->MakeCockpit();
      shape->OrSpecial(BestMipmap);
    }
    const ParamEntry &par = *_parClass;
    _animFire.Init(shape, (par >> "selectionFireAnim").operator RString(), NULL);
  }
}

void MuzzleType::LODShapeUnloaded(LODShape *shape)
{
  if (shape==_opticsModel)
  {
    _animFire.Deinit(shape);
  }
}

void MuzzleType::ShapeLoaded(LODShape *shape, int level)
{
  if (shape==_opticsModel)
  {
    _animFire.InitLevel(shape, level);
  }
}

void MuzzleType::ShapeUnloaded(LODShape *shape, int level)
{
  if (shape==_opticsModel)
  {
    _animFire.DeinitLevel(shape, level);
  }
}

void MuzzleType::DeinitShape(const WeaponType *weapon)
{
  if (_opticsModel)
  {
    _opticsModel->RemoveLoadHandler(this);
    _opticsModel.Free();
  }

  _cursorTexture.texture.Free();
  _cursorTexture.sections.Clear();
  _cursorAimTexture.texture.Free();
  _cursorAimTexture.sections.Clear();
}

void MuzzleType::InitShapeLevel(const WeaponType *weapon, int level)
{
}

void MuzzleType::DeinitShapeLevel(const WeaponType *weapon, int level)
{
}

bool MuzzleType::CanUse(const MagazineType *type) const
{
  for (int i=0; i<_magazines.Size(); i++)
  {
    if (_magazines[i] == type)
      return true;
  }
  return false;
}

// class WeaponType
WeaponType::WeaponType(ParamEntryPar param)
  : base(param)
{
  _shotFromTurret = false;
  _bow = false;
  _mass = 0;
}

#include <Es/Common/delegate.hpp>

static const struct AnimSourceDesc
{
    const char *name;
    float (Weapon::*func)(ObjectVisualState const&) const;
} desc[] =
{
    {"revolving", &Weapon::GetRevolvingPos},
    {"reload", &Weapon::GetReloadPos},
    {"reloadmagazine", &Weapon::GetMagazineReloadPos},
    {"isempty", &Weapon::IsEmpty},
    {"drawbow", &Weapon::GetDrawBowPos},
#if _VBS2
    {"ammo", &Weapon::GetAmmo},
#endif
};

AnimationSource *WeaponType::CreateAnimationSource(const AnimationType *type, RStringB source)
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

void WeaponType::InitShape()
{
  base::InitShape();

  //LogF("Init weapon %s",(const char *)GetName());

  if (_shape)
  {
    //LogF("Loading weapon model %s - %s",(const char *)GetName(),(const char *)wModelName);
    //bool shadow = true;
    // TODO: use shadow member of CfgNonAIVehicles::ProxyWeapon
    //(*_parClass)>>"shadow";
    _shape->SetAutoCenter(false);
    // do not change minmax and bounding radius, there are loaded and not enough info is available now
    _shape->CalculateBoundingSphere(false);
    const ParamEntry &par = *_par;
    _animFire.Init(_shape, (par >> "selectionFireAnim").operator RString(), NULL);

    if (_shape->GetSkeletonSourceName().GetLength() == 0)
    {
      _shape->SetAnimationType(AnimTypeNone);
    }
    else
    {
      _shape->SetAnimationType(AnimTypeHardware);
      _shape->LoadSkeletonFromSource();
    }

    ConstParamEntryPtr array = _par->FindEntry("Animations");
    if (array)
    {
      RptF("Obsolete class Animations defined in %s",cc_cast(GetName()));
    }
    _animSources.Load(this, GetAnimations());
  }
  for (int j=0; j<_muzzles.Size(); j++)
  {
    MuzzleType *muzzle = _muzzles[j];
    muzzle->InitShape(this);
  }
}

void WeaponType::DeinitShape()
{
  for (int j=0; j<_muzzles.Size(); j++)
  {
    MuzzleType *muzzle = _muzzles[j];
    muzzle->DeinitShape(this);
  }
  //LogF("Deinit weapon %s",(const char *)GetName());
  if (_shape)
  {
    _animFire.Deinit(_shape);

    _animSources.Clear();
  }

  base::DeinitShape();
}

void WeaponType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);

  if (_shape)
  {
    _animFire.InitLevel(_shape, level);
  }
  for (int j=0; j<_muzzles.Size(); j++)
  {
    MuzzleType *muzzle = _muzzles[j];
    muzzle->InitShapeLevel(this,level);
  }
}

void WeaponType::DeinitShapeLevel(int level)
{
  for (int j=0; j<_muzzles.Size(); j++)
  {
    MuzzleType *muzzle = _muzzles[j];
    muzzle->DeinitShapeLevel(this,level);
  }
  if (_shape)
  {
    _animFire.DeinitLevel(_shape, level);
  }

  base::DeinitShapeLevel(level);
}

RStringB WeaponType::GetPictureName() const
{
  return _picName.GetLength()>0 ? _picName : GetName();
}

/*!
\patch_internal 1.30 Date 11/02/2001 by Jirka
- Added: parameter "shotFromTurret"
*/
void WeaponType::Load(ParamEntryPar cls, const char *shape)
{
  base::Load(cls, shape);

  // ERROR_CATCH_ANY can be executed multiple times, but we want only one error message
  bool errorOnce = false;
  ERROR_TRY()
  
  _scope = cls >> "scope";
/*
  if (_scope == 0)
  {
    WarningMessage("Error: creating weapon %s with scope=private", cc_cast(cls.GetName()));
  }
*/
  _weaponType = cls >> "type";
  _picName = cls>>"picture";
  _description = cls >> "Library" >> "libTextDesc";

  RStringB simName = cls >> "simulation";
  _simulation = GetEnumValue<WeaponSimulation>(simName);

  _fireLightDuration = cls >> "fireLightDuration";
  _fireLightIntensity = cls >> "fireLightIntensity";

  _mass = cls >> "weight";

  if (cls.FindEntry("weaponInfoType")) 
  {
    ParamEntryVal types = cls >> "weaponInfoType";
    if(types.IsArray())
    {
      int n = types.GetSize();
      _weaponInfoTypes.Realloc(n);
      _weaponInfoTypes.Resize(n);
      for (int i=0; i<n; i++) _weaponInfoTypes[i] = types[i];
    }

    else
    {
      _weaponInfoTypes.Realloc(1);
      _weaponInfoTypes.Resize(1);
      _weaponInfoTypes[0] = types;
    }
  }

#if _VBS2 // fatigue model
  _lightMode = (int)cls.ReadValue("lightmode",0.0f);
#endif

  ConstParamEntryPtr entry = cls.FindEntry("shotFromTurret");
  if (entry) _shotFromTurret = *entry;
  else _shotFromTurret = false;

  RString picture;
  entry = cls.FindEntry("uiPicture");
  if (entry) picture = *entry;
  if (picture.GetLength() > 0) _picture = GlobLoadTexture(::GetPictureName(picture));

#if _ENABLE_DATADISC
  entry = cls.FindEntry("canDrop");
  if (entry) _canDrop = *entry;
  else _canDrop = true;
#else
  _canDrop = false;
#endif

  entry = cls.FindEntry("dexterity");
  if (entry) _dexterity = *entry;
  else _dexterity = 1;

  entry = cls.FindEntry("value");
  if (entry) _value = *entry;
  else _value = 1;

  RString animClassLow(AnimClass());
  animClassLow.Lower();
  _bow = (animClassLow == "bow");

  int n = (cls >> "muzzles").GetSize();
  _muzzles.Resize(n);
  for (int i=0; i<n; i++)
  {
    RStringB muzzle = (cls >> "muzzles")[i];
    ParamEntryPar muzzleCls = stricmp(muzzle, "this") == 0 ? cls : (cls >> muzzle);
    _muzzles[i] = new MuzzleType();
    _muzzles[i]->Init(muzzleCls, this);
  }
  ERROR_CATCH_ANY(exc)
    if (!errorOnce)
    {
      errorOnce = true;
      LogF("Error loading WeaponType %s",cc_cast(cls.GetContext()));
    }
  ERROR_END()
}

Object* WeaponType::CreateObject(bool fullCreate) const
{ 
  return new Weapon(this, fullCreate);
}

void Muzzle::Init(const MuzzleType *type)
{
  if (type && type->HasVisionModes(_opticsMode))
  {
    type->GetInitVisionMode(_visionPars._visionMode, _visionPars._flirIndex, _opticsMode);
  }
  else
  {
    _visionPars._flirIndex = 0;
    _visionPars._visionMode = VMNormal;
  }

  if (type)
  {
    _discreteIndex = type->_opticsInfo[_opticsMode]._discreteInitIndex;
    _discreteDistanceIndex  = type->_opticsInfo[_opticsMode]._discreteDistanceInitIndex;
  }
  else
  {
    _discreteIndex = 0;
    _discreteDistanceIndex = 0;
  }
}

bool Muzzle::IsDefaultValue(ParamArchive &ar) const
{
  if (_magazine) return false;
  if (_magazineWanted) return false;
  if (_opticsMode != 0) return false;
  if (_discreteIndex != 0) return false;
  if (_discreteDistanceIndex != 0) return false;
  if (!_visionPars.IsDefaultValue(ar)) return false;
  if (_currentMode != -1) return false;

  return true;
}

LSError Muzzle::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeRef("Magazine", _magazine, 1));
  CHECK(ar.SerializeRef("MagazineWanted", _magazineWanted, 1));

  CHECK(ar.Serialize("opticsMode", _opticsMode, 1, 0))
  CHECK(ar.Serialize("discreteIndex", _discreteIndex, 1, 0))
  CHECK(ar.Serialize("discreteDistanceIndex", _discreteDistanceIndex, 1, 0))
  CHECK(ar.SerializeDef("VisionPars", _visionPars, 1))
  CHECK(ar.Serialize("currentMode", _currentMode, 1, -1))

  return LSOK;
}

void Muzzle::LoadDefaultValues(ParamArchive &ar)
{
  _magazine = NULL;
  _magazineWanted = NULL;
  _opticsMode = 0;
  _discreteIndex = 0;
  _discreteDistanceIndex = 0;
  _visionPars.LoadDefaultValues(ar);
  _currentMode = -1;
}

DEFINE_NETWORK_OBJECT_SIMPLE(Muzzle, UpdateMuzzle)

DEFINE_NET_INDICES(UpdateMuzzle, UPDATE_MUZZLE_MSG)

NetworkMessageFormat &Muzzle::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  UPDATE_MUZZLE_MSG(UpdateMuzzle, MSG_FORMAT)
  return format;
}

TMError Muzzle::TransferMsg(NetworkMessageContext &ctx)
{
  PREPARE_TRANSFER(UpdateMuzzle)

  TRANSF_REF(magazineWanted)
  TRANSF_REF(magazine)
  TRANSF(currentMode)

  return TMOK;
}

Weapon::Weapon(const EntityAIType *type, bool fullCreate) : base(type, fullCreate)
{
  _phaseFire = -1;
  _gunHeatFactor = 0.0f;

  _currentMuzzle = 0;

  const WeaponType *weaponType = static_cast<const WeaponType *>(type);
  if (weaponType)
  {
    int n = weaponType->_muzzles.Size();
    _muzzles.Realloc(n);
    _muzzles.Resize(n);
    for (int i=0; i<n; i++)
      _muzzles[i].Init(weaponType->_muzzles[i]);
  }
}

bool Weapon::RemoveOwnedItem(InventoryItem *item)
{
  bool found = false;
  for (int i=0; i<_muzzles.Size(); i++)
  {
    if (_muzzles[i]._magazine == item)
    {
      _muzzles[i]._magazine = NULL;
      found = true;
    }
    if (_muzzles[i]._magazineWanted == item)
    {
      _muzzles[i]._magazineWanted = _muzzles[i]._magazine;
    }
  }
  if (found) return true;

  return base::RemoveOwnedItem(item);
}

bool Weapon::PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level)
{
  if (!shape) return false;

  /// prepare identity matrix array
  shape->InitDrawMatrices(matrices, level);

  const WeaponType *type = Type();
  if (type)
  {
    return type->_animSources.PrepareShapeDrawMatrices(matrices, shape, level, vs, this, type->GetAnimations());
  }
  return false;
}

AnimationStyle Weapon::IsAnimated(int level) const
{
  return AnimatedGeometry;
}

bool Weapon::CanBeInstanced(int level, float distance2, const DrawParameters &dp) const
{
  return false;
}

void Weapon::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);

  LODShape *shape = NULL;
  AnimationAnimatedTexture *anim = NULL;

  // try to use _modelSpecial from the current magazine
  if (_currentMuzzle >= 0 && _currentMuzzle < _muzzles.Size())
  {
    const Muzzle &muzzle = _muzzles[_currentMuzzle];
    const Magazine *magazine = muzzle._magazine;
    if (magazine && magazine->Type() && magazine->GetAmmo() > 0)
    {
      LODShape *model = magazine->Type()->_modelSpecial;
      if (model)
      {
        shape = model;
        anim = &magazine->Type()->_animFire;
      }
    }
  }

  // otherwise, use the shape from type
  if (!shape)
  {
    const WeaponType *type = Type();
    if (!type) return;
    shape = type->GetShape();
    anim = &type->_animFire;
  }

  if (_phaseFire >= 0)
  {
    anim->Unhide(animContext, shape, level);
    anim->SetPhaseLoadedOnly(animContext, shape, level, _phaseFire);
  }
  else
  {
    anim->Hide(animContext, shape, level);
  }
}

void Weapon::Deanimate(int level, bool setEngineStuff)
{
}

void Weapon::SetAnimPhase(int phaseFire)
{
  _phaseFire = phaseFire;
}

void Weapon::AddHeat()
{
  // Shot occurred, add some heat
  static float fireHeatDelta = 1.0f / 30.0f;

  _gunHeatFactor += fireHeatDelta;
  saturateMin(_gunHeatFactor, 1.0f);
}

/// functor searching for weapon in all weapon systems
class FindTurretWeapon : public ITurretFunc
{
protected:
  EntityAIFull *&_entity;
  WeaponsState *&_state;
  Weapon *_weapon;

public:
  FindTurretWeapon(EntityAIFull *&entity, WeaponsState *&state, Weapon *weapon) : _entity(entity), _state(state), _weapon(weapon) {}
  bool operator () (EntityAIFull *entity, TurretContext &context)
  {
    return context._weapons->ForEachWeapon([this, entity, &context](Weapon *weapon) -> bool
    {
      if (weapon == _weapon)
      {
        _entity = entity;
        _state = context._weapons;
        return true; // done
      }
      return false; // continue
    });
  }
  EntityAIFull *GetEntity() {return _entity;}
  WeaponsState *GetState() {return _state;}
};

void Weapon::Simulate(float deltaT, SimulationImportance prec)
{
  // check if reload is required
  bool reloadNeeded = false;
  for (int i=0; i<_muzzles.Size(); i++)
  {
    Muzzle &muzzle = _muzzles[i];
    if (muzzle._magazine != muzzle._magazineWanted)
    {
      if (muzzle._magazineWanted.NotNull())
        reloadNeeded = true;
      else
        muzzle._magazine = NULL; // no process attached to magazine removal
    }
  }
  if (reloadNeeded)
  {
    // find who can reload
    Entity *root = this;
    while (root->GetHierarchyParent())
    {
      root = root->GetHierarchyParent();
      EntityAIFull *entity = NULL;
      WeaponsState *state = NULL;
      FindTurretWeapon func(entity, state, this);
      if (root->ForEachTurret(func))
      {
        // reload all muzzles
        for (int i=0; i<_muzzles.Size(); i++)
        {
          Muzzle &muzzle = _muzzles[i];
          if (muzzle._magazine != muzzle._magazineWanted)
          {
            WeaponModePtr ptr;
            ptr._weapon = this; ptr._muzzle = i; ptr._mode = muzzle._currentMode;
            func.GetEntity()->DoReloadMagazine(*func.GetState(), ptr);
          }
        }
        // handled
        root = nullptr;
        break;
      }
    }
    // not handled
    if (root)
    {
      // load magazine immediately for weapon on ground / in cargo
      for (int i=0; i<_muzzles.Size(); i++)
      {
        Muzzle &muzzle = _muzzles[i];
        if (muzzle._magazine != muzzle._magazineWanted)
        {
          if (muzzle._magazine)
          {
            muzzle._magazine->ItemDrop(nullptr, root->FutureVisualState().Position(), root->FutureVisualState().Direction());
            muzzle._magazine = nullptr;
          }
          muzzle._magazine = muzzle._magazineWanted;
        }
      }
    }
  }

  // weapon cooling
  static float invTimeToCoolDown = 1.0f / (60.0f * 4.0f);
  float value = deltaT * invTimeToCoolDown;

  _gunHeatFactor -= value;
  saturateMax(_gunHeatFactor, 0.0f);

  base::Simulate(deltaT, prec);
}

bool Weapon::UseFromInventory(Person *owner)
{
  DoAssert(Type() && Type()->IsHandheld());
  return base::UseFromInventory(owner);
}

bool Weapon::CanAutoShortcut() const
{
  return true;
}

float Weapon::GetRevolvingPos(const ObjectVisualState &vs) const
{
  if (_muzzles.Size() == 0) return 0;
  // TODO: support for other muzzles
  const Magazine *magazine = _muzzles[0]._magazine;
  if (!magazine) return 0;

  float reload = magazine->_reload;
  saturate(reload, 0, 1);
  return (magazine->GetAmmo() + reload) / magazine->Type()->_maxAmmo;
}

float Weapon::GetReloadPos(const ObjectVisualState &vs) const
{
  if (_muzzles.Size() == 0) return 0;
  // TODO: support for other muzzles
  const Magazine *magazine = _muzzles[0]._magazine;
  if (!magazine) return 0;

  float reload = magazine->_reload;
  saturate(reload, 0, 1);
  return reload;
}

float Weapon::IsEmpty(const ObjectVisualState &vs) const
{
  if (_muzzles.Size() == 0) return 0;
  // TODO: support for other muzzles
  const Magazine *magazine = _muzzles[0]._magazine;
  if (!magazine) return 0;

  return magazine->GetAmmo() > 0 ? 0.0f : 1.0f;
}

float Weapon::GetDrawBowPos(const ObjectVisualState &vs) const
{
  if (_muzzles.Size() == 0) return 0;
  const Muzzle &muzzle = _muzzles[0];
  float byAnim = muzzle._drawBowProgress;
  saturate(byAnim, 0.f, 1.f);
  return byAnim;
}

const OpticsInfo *Weapon::GetAttachedOpticsInfo() const
{
  for (int i=0; i<_attachments.Size(); i++)
  {
    if (_attachments[i])
    {
      ItemOptics *optics = _attachments[i]->GetOptics();
      if (optics)
      {
        return &optics->Type()->_opticInfo;
      }
    }
  }
  return nullptr;
}

const float Weapon::SumDispersionModifiers(const EntityAIFull *agent) const
{
  float dispersionSum = 0;
  for (int i=0; i<_attachments.Size(); i++)
  {
    if (_attachments[i] && CheckConditionDispersion(_attachments[i], agent))
    {
      dispersionSum += _attachments[i]->Type()->GetDispersionModifier();
    }
  }
  return dispersionSum;
}

const float Weapon::SumDexterityModifiers(const EntityAIFull *agent) const
{
  float dexteritySum = 0;
  for (int i=0; i<_attachments.Size(); i++)
  {
    if (_attachments[i] && CheckConditionDexterity(_attachments[i], agent))
    {
      dexteritySum += _attachments[i]->Type()->GetDexterityModifier();
    }
  }
  return dexteritySum;
}

ItemOptics *Weapon::GetAttachedOptics()
{
  for (int i=0; i<_attachments.Size(); i++)
  {
    if (_attachments[i])
    {
      ItemOptics *optics = _attachments[i]->GetOptics();
      if (optics) return optics;
    }
  }
  return nullptr;
}

float Weapon::GetMagazineReloadPos(const ObjectVisualState &vs) const
{
  if (_muzzles.Size() == 0) return 0;
  // TODO: support for other muzzles
  const Muzzle &muzzle = _muzzles[0];
  float byAnim = muzzle._reloadMagazineProgress;

  const Magazine *magazine = muzzle._magazine;
  if (magazine)
  {
    float byTime = magazine->_reloadMagazineTotal>0 ? magazine->_reloadMagazine / magazine->_reloadMagazineTotal : 0;
    // combine both ways of animation
    byAnim = floatMin(byTime+byAnim,1);
  }
  return byAnim;
}

void Weapon::UpdateShape()
{
  // try to use _modelSpecial from the current magazine
  if (_currentMuzzle >= 0 && _currentMuzzle < _muzzles.Size())
  {
    const Muzzle &muzzle = _muzzles[_currentMuzzle];
    const Magazine *magazine = muzzle._magazine;
    if (magazine && magazine->Type() && magazine->GetAmmo() > 0)
    {
      LODShape *shape = magazine->Type()->_modelSpecial;
      if (shape)
      {
        _shape = shape;
        return;
      }
    }
  }

  // otherwise, use the shape from type
  const WeaponType *type = Type();
  if (type) _shape = type->GetShape();
}

int Weapon::NAttachments() const
{
  return base::NAttachments() + _muzzles.Size();
}

Texture *WeaponType::GetAttachmentIcon(int i) const
{
  int n = GetAttachments().Size();
  if (i < n) return base::GetAttachmentIcon(i);
  i -= n;
  if (i >= _muzzles.Size()) return nullptr;
  return _muzzles[i]->_magazineSlotIcon;
}

const InventoryItem *Weapon::GetAttachment(int i) const
{
  int n = base::NAttachments();
  if (i < n) return base::GetAttachment(i);
  i -= n;
  if (i >= _muzzles.Size()) return nullptr;
  return _muzzles[i]._magazine;
}

InventoryItem *Weapon::GetAttachment(int i)
{
  int n = base::NAttachments();
  if (i < n) return base::GetAttachment(i);
  i -= n;
  if (i >= _muzzles.Size()) return nullptr;
  return _muzzles[i]._magazine;
}

InventoryItem *Weapon::GetAttachment(RString slotName)
{
  const WeaponType *type = Type();
  for (int j=0; j<type->_muzzles.Size(); j++)
  {
    if (stricmp(slotName, type->_muzzles[j]->GetMagazineSlot()) == 0) return _muzzles[j]._magazine;
  }

  // common attachment
  return base::GetAttachment(slotName);
}

InventoryItem *Weapon::FindAttachment(InventorySlotId slotId) const
{
  int index = Type()->GetAttachments().FindKey(slotId);
  if (index >= 0) return _attachments[index];

  // check the special slots
  if (slotId < 0 || slotId >= InventorySlotTypes.FirstInvalidValue()) return nullptr;
  RString slotName = InventorySlotTypes.GetName(slotId);

  const WeaponType *type = Type();
  for (int j=0; j<type->_muzzles.Size(); j++)
  {
    if (stricmp(slotName, type->_muzzles[j]->GetMagazineSlot()) == 0) return _muzzles[j]._magazine;
  }

  return nullptr;
}

bool Weapon::CanAddAttachment(const InventoryItem *item, int index) const
{
  int n = base::NAttachments();
  if (index < n) return base::CanAddAttachment(item, index);
  index -= n;
  if (index >= _muzzles.Size()) return false;

  if (_muzzles[index]._magazine) return false; // occupied
  if (_muzzles[index]._magazineWanted) return false; // loading
  
  // check if magazine matches
  const WeaponType *type = Type();
  if (!type) return false;
  const Magazine *magazine = item->GetMagazine();
  if (!magazine) return false;
  const MagazineType *magazineType = magazine->Type();
  if (!magazineType) return false;

  const MuzzleType *muzzleType = type->_muzzles[index];
  return muzzleType->_magazines.Find(magazineType) >= 0;
}

bool Weapon::AddAttachment(InventoryItem *item, int index)
{
  int n = base::NAttachments();
  if (index < n) return base::AddAttachment(item, index);
  index -= n;
  if (index >= _muzzles.Size()) return false;

  if (_muzzles[index]._magazine) return false; // occupied
  if (_muzzles[index]._magazineWanted) return false; // loading
  
  // check if magazine matches
  const WeaponType *type = Type();
  if (!type) return false;
  Magazine *magazine = item->GetMagazine();
  if (!magazine) return false;
  const MagazineType *magazineType = magazine->Type();
  if (!magazineType) return false;

  const MuzzleType *muzzleType = type->_muzzles[index];
  if (muzzleType->_magazines.Find(magazineType) < 0) return false;

  // move in world
  item->ItemMoveOut(this);

  _muzzles[index]._magazineWanted = magazine;
  return true;
}

bool Weapon::CanAddAttachment(const InventoryItem *item) const
{
  if (item->IsDamageDestroyed()) return false;

  const WeaponType *type = Type();
  if (type)
  {
    // for now, only magazines are supported
    const Magazine *magazine = item->GetMagazine();
    if (magazine)
    {
      const MagazineType *magazineType = magazine->Type();
      if (magazineType)
      {
        for (int i=0; i<type->_muzzles.Size(); i++)
        {
          const Muzzle &muzzle = _muzzles[i];
          if (muzzle._magazine) continue; // not empty
          if (muzzle._magazineWanted) continue; // loading

          // check if fits into
          const MuzzleType *muzzleType = type->_muzzles[i];
          for (int j=0; j<muzzleType->_magazines.Size(); j++)
          {
            if (magazineType == muzzleType->_magazines[j])
            {
              // fits here
              return true;
            }
          }
        }
      }
    }
  }

  return base::CanAddAttachment(item);
}

bool Weapon::AddAttachment(InventoryItem *item)
{
  const WeaponType *type = Type();
  if (type)
  {
    // for now, only magazines are supported
    Magazine *magazine = item->GetMagazine();
    if (magazine)
    {
      const MagazineType *magazineType = magazine->Type();
      if (magazineType)
      {
        for (int i=0; i<type->_muzzles.Size(); i++)
        {
          Muzzle &muzzle = _muzzles[i];
          if (muzzle._magazine) continue; // not empty
          if (muzzle._magazineWanted) continue; // loading

          // check if fits into
          const MuzzleType *muzzleType = type->_muzzles[i];
          for (int j=0; j<muzzleType->_magazines.Size(); j++)
          {
            if (magazineType == muzzleType->_magazines[j])
            {
              // fits here
              magazine->ItemMoveOut(this); // move in world
              muzzle._magazineWanted = magazine;
              return true;
            }
          }
        }
      }
    }
  }

  return base::AddAttachment(item);
}

LSError Weapon::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))

  CHECK(ar.SerializeDef("Muzzles", _muzzles, 1))
  CHECK(ar.Serialize("currentMuzzle", _currentMuzzle, 1, -1))
  CHECK(ar.Serialize("gunHeatFactor", _gunHeatFactor, 1, 0.0f))

  return LSOK;
}

Weapon *Weapon::CreateObject(ParamArchive &ar)
{
  return dyn_cast<Weapon>(base::CreateObject(ar));
}

#define WEAPON_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateWeapon)

DEFINE_NETWORK_OBJECT(Weapon, base, WEAPON_MSG_LIST)

DEFINE_NET_INDICES_EX_ERR(UpdateWeapon, UpdateInventoryItem, UPDATE_WEAPON_MSG, NoErrorInitialFunc)

NetworkMessageFormat &Weapon::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCUpdateDamage:
    base::CreateFormat(cls, format);
    break;
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_WEAPON_MSG(UpdateWeapon, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError Weapon::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateDamage:
    TMCHECK(base::TransferMsg(ctx))
    break;
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateWeapon)

      TRANSF_ARRAY(muzzles)
      TRANSF(currentMuzzle)
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float CalculateMuzzleError(const RefArray<NetworkMessage> &value1, const RefArray<NetworkMessage> &value2)
{
  float error = 0;

  int n = intMin(value1.Size(), value2.Size()); // number of muzzles should be the same
  for (int i=0; i<n; i++)
  {
    const NetworkMessageUpdateMuzzle *msg1 = (const NetworkMessageUpdateMuzzle *)(value1[i].GetRef());
    const NetworkMessageUpdateMuzzle *msg2 = (const NetworkMessageUpdateMuzzle *)(value2[i].GetRef());
    if (!msg1 || !msg2) continue;

    if (msg1->_currentMode != msg1->_currentMode) error += ERR_COEF_MODE;
    if (msg1->_magazineWanted != msg2->_magazineWanted) error += ERR_COEF_MODE;
  }

  return error;
}

float Weapon::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateDamage:
    {
      error += base::CalculateError(ctx);
    }
    break;
  case NMCUpdateGeneric:
    {
      error += base::CalculateError(ctx);
      {
        PREPARE_TRANSFER(UpdateWeapon)

        // muzzles
        int n = intMin(message->_muzzles.Size(), _muzzles.Size()); // number of muzzles should be the same
        for (int i=0; i<n; i++)
        {
          const NetworkMessageUpdateMuzzle *msg = (const NetworkMessageUpdateMuzzle *)(message->_muzzles[i].GetRef());
          if (!msg) continue;
          const Muzzle &muzzle = _muzzles[i];

          if (msg->_currentMode != muzzle._currentMode) error += ERR_COEF_MODE;

          NetworkId mag = muzzle._magazineWanted ? muzzle._magazineWanted->GetNetworkId() : NetworkId::Null();
          if (mag != msg->_magazineWanted) error += ERR_COEF_MODE;
        }

        ICALCERR_NEQ(int, currentMuzzle, ERR_COEF_MODE)
      }
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}

void Weapon::CreateSubobjects()
{
  for (int i=0; i<_muzzles.Size(); i++)
  {
    Magazine *magazine = _muzzles[i]._magazine;
    if (magazine)
      GetNetworkManager().CreateVehicle(magazine, VLTOut, "", -1);
    Magazine *magazineWanted = _muzzles[i]._magazineWanted;
    if (magazineWanted && magazineWanted != magazine) 
      GetNetworkManager().CreateVehicle(magazineWanted, VLTOut, "", -1);
  }
  base::CreateSubobjects();
}

void Weapon::UpdateSubobjects()
{
  for (int i=0; i<_muzzles.Size(); i++)
  {
    Magazine *magazine = _muzzles[i]._magazine;
    if (magazine)
    {
      magazine->UpdateSubobjects();
      GetNetworkManager().UpdateObject(magazine);
    }
    Magazine *magazineWanted = _muzzles[i]._magazineWanted;
    if (magazineWanted && magazineWanted != magazine)
    {
      magazineWanted->UpdateSubobjects();
      GetNetworkManager().UpdateObject(magazineWanted);
    }
  }
  base::CreateSubobjects();
}

void Weapon::JSONSave(JSONValue &container, JSONAllocator &allocator) const
{
  base::JSONSave(container, allocator);

  JSONValue newItems;
  if (!container.HasMember("items"))
  {
    newItems.SetArray();
    container.AddMember("items", newItems, allocator);
  }

  JSONValue &items = container["items"];
  if (items.IsArray())
  {
    const WeaponType *type = Type();
    for (int i=0; i<type->_muzzles.Size(); i++)
    {
      // TODO: other attachments
      Magazine *magazine = _muzzles[i]._magazineWanted;
      if (magazine)
      {
        JSONValue value;
        value.SetObject();
        JSONValue slot;
        slot.SetString(type->_muzzles[i]->GetMagazineSlot(), allocator);
        value.AddMember("slot", slot, allocator);
        value.AddMember("type", magazine->GetName(), allocator);
        magazine->JSONSave(value, allocator);
        items.PushBack(value, allocator);
      }
    }
  }
}

bool Weapon::JSONLoad(JSONValue &container, Person *person)
{
  if (!base::JSONLoad(container, person)) return false;

  JSONValue &items = container["items"];
  if (items.IsArray())
  {
    const WeaponType *type = Type();

    for (int i=0; i<items.Size(); i++)
    {
      JSONValue &value = items[i];
      if (value.IsObject())
      {
        RString slot = value["slot"].GetString();
        for (int j=0; j<type->_muzzles.Size(); j++)
        {
          if (stricmp(slot, type->_muzzles[j]->GetMagazineSlot()) == 0)
          {
            RString type = value["type"].GetString();
            Ref<Magazine> magazine = dyn_cast<Magazine>(GWorld->NewNonAIVehicleWithID(type, RString()));
            if (magazine && magazine->JSONLoad(value, person))
            {
              _muzzles[j]._magazineWanted = magazine;
              AddAttachment(magazine);
              _muzzles[j]._magazine = magazine;

              // add to the world
              GWorld->AddOutVehicle(magazine);
              magazine->SetMoveOutDone(this);

              // DO NOT create network object as it will be created later in CreateSubobjects during CreateVehicle of its hierParent
            }
            break;
          }
        }
      }
    }
  }

  return true;
}

DEFINE_CASTING(Weapon)
