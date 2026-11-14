#include "../wpch.hpp"
#include "itemoptics.hpp"
#include <lib/fileLocator.hpp>

void ItemOpticsType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par, shape);

  // OpticsInfo
  ParamEntryPtr entry = par.FindEntry("OpticsInfo");
  if (entry)
  {
    // modelOptics
    _oModelName = *entry >> "modelOptics";

    ParamEntryPtr cameraMemoryPointEntry = par.FindEntry("memoryPointCamera");
    if (cameraMemoryPointEntry) _opticInfo._memoryPointCamera = *cameraMemoryPointEntry;

    _opticInfo._opticsZoomMin = *entry >> "opticsZoomMin";
    _opticInfo._opticsZoomMax = *entry >> "opticsZoomMax";
    _opticInfo._opticsZoomInit = *entry >> "opticsZoomInit";
    _opticInfo._distanceZoomMin = *entry >> "distanceZoomMin";
    _opticInfo._distanceZoomMax = *entry >> "distanceZoomMax";

    ParamEntryVal discreteDist = *entry >> "discreteDistance";
    if (discreteDist.IsArray())
    {
      if(discreteDist.GetSize() > 0)
      {
        _opticInfo._discreteDistance.Realloc(discreteDist.GetSize());
        _opticInfo._discreteDistance.Resize(discreteDist.GetSize());
        for (int i=0; i<discreteDist.GetSize(); i++)
          _opticInfo._discreteDistance[i] = discreteDist[i];
      }
      _opticInfo._discreteDistanceInitIndex = *entry >> "discreteDistanceInitIndex";
    }

    _opticInfo._opticsDisablePeripherialVision = *entry >> "opticsDisablePeripherialVision";
    _opticInfo._opticsFlare = *entry >> "opticsFlare";
    _opticInfo._useModelOptics = true;
    _opticInfo._id = 0; //default is -1
    ParamEntryVal ppeEntry = *entry >> "opticsPPEffects";
    if (ppeEntry.IsArray())
    {
      ParamEntryVal cfgOpticsPars = Pars >> "CfgOpticsEffect";
      int size = ppeEntry.GetSize();
      _opticInfo._ppEffectType.Realloc(size);
      _opticInfo._ppEffectType.Resize(size);

      for (int i = 0; i < size; i++)
      {
        // effect type
        RString name = ppeEntry[i];
        if (cfgOpticsPars.FindEntry(name))
        {
          ParamEntryVal eEntry = cfgOpticsPars >> name;
          if (eEntry.IsClass())
            _opticInfo._ppEffectType[i].Load(eEntry);
        }
      }
    }
  }
}

Object* ItemOpticsType::CreateObject(bool fullCreate) const
{
  return new ItemOptics(this, fullCreate);
}

void ItemOpticsType::InitShape()
{
  base::InitShape();

  if (_oModelName.GetLength()>0)
  {
    static RStringB noModel("-");
    if (_oModelName!=noModel && !_oModelName.IsEmpty())
    {
      _opticsModel = Shapes.New(::GetShapeName(_oModelName),true,false);
      _opticsModel->AddLoadHandler(this);
    }
  }
}

void ItemOpticsType::DeinitShape()
{
  if (_opticsModel)
  {
    _opticsModel->RemoveLoadHandler(this);
    _opticsModel.Free();
  }

  base::DeinitShape();
}

void ItemOpticsType::LODShapeLoaded(LODShape *shape)
{
  if (shape==_opticsModel)
  {
    if (_opticsModel->NLevels() > 0)
    {
      _opticsModel->Level(0)->MakeCockpit();
      _opticsModel->OrSpecial(BestMipmap);
    }

    const ParamEntry &par = *_par;
    _animFire.Init(shape, (par >> "selectionFireAnim").operator RString(), NULL);
  }
  else
  { //Note: base should be called for _shape (as we are EntityType opposed to MuzzleType or ClothingType)
    //      must not be called for _opticsModel, as InitShape is called inside and AddLoadHandler calls LODShapeLoaded i.e. us again => Stack Overflow
    base::LODShapeLoaded(shape);
  }
}

void ItemOpticsType::LODShapeUnloaded(LODShape *shape)
{
  if (shape==_opticsModel)
  {
    _animFire.Deinit(shape);
    _opticsModel.Free();
  }
  else base::LODShapeUnloaded(shape);
}

void ItemOpticsType::ShapeLoaded(LODShape *shape, int level)
{
  if (shape==_opticsModel)
  {
    _animFire.InitLevel(shape, level);
  }
  else base::ShapeLoaded(shape, level);
}

void ItemOpticsType::ShapeUnloaded(LODShape *shape, int level)
{
  if (shape==_opticsModel)
  {
    _animFire.DeinitLevel(shape, level);
  }
  else base::ShapeUnloaded(shape, level);
}
