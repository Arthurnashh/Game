#include "../wpch.hpp"
#include "clothing.hpp"
#include <lib/fileLocator.hpp>
#include <lib/person.hpp>
#include <lib/AI/ai.hpp>

DynEnum ClothingTypes;

void ClothingType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par, shape);

  ParamEntryVal cls = par >> "ClothingTypes";
  for (int i=0; i<cls.GetEntryCount(); i++)
  {
    ParamEntryVal entry = cls.GetEntry(i);
    int clothingType = ClothingTypes.AddValue(entry.GetName(), false);
    if (_clothingTypes.Size() < clothingType + 1) _clothingTypes.Resize(clothingType + 1);
    RString model = entry.GetValue();
    if (!model.IsEmpty()) _clothingTypes[clothingType] = ::GetShapeName(model);
  }

  int n = _clothingTypes.Size();
  _wearModels.Realloc(n); _wearModels.Resize(n);
  for (int i=0; i<n; i++) _wearModels[i] = new WearModel(this);
}

void ClothingType::InitShape()
{
  base::InitShape();

  for (int i=0; i<_clothingTypes.Size(); i++)
  {
    RString name = _clothingTypes[i];
    WearModel *model = _wearModels[i];
    if (name.IsEmpty())
      model->_model.Free();
    else
    {
      LODShape *shape = Shapes.New(::GetShapeName(name), ShapeReversed);
      model->_model = shape;
      if (shape) shape->AddLoadHandler(model); // safe to reference, does not change during lifetime of ClothingType
    }
  }
}

void ClothingType::DeinitShape()
{
  for (int i=0; i<_wearModels.Size(); i++)
  {
    WearModel *model = _wearModels[i];
    if (model->_model)
    {
      model->_model->RemoveLoadHandler(model);
      model->_model.Free();
    }
  }

  base::DeinitShape();
}

void ClothingType::WearModel::LODShapeLoaded(LODShape *shape)
{
  // handle other shapes ourself
  _hiddenSelections.Clear();

  if (shape)
  {
    shape->LoadSkeletonFromSource();

    const ParamEntry &par = *_owner->_par;

    // hidden selections
    ParamEntryVal hiddenSelections = par >> "hiddenSelections";
    int n = hiddenSelections.GetSize();
    _hiddenSelections.Realloc(n);
    _hiddenSelections.Resize(n);
    for (int i=0; i<n; i++)
    {
      RString name = hiddenSelections[i];
      _hiddenSelections[i].Init(shape, name, NULL);
    }
    // Lights
    EntityAIType *type = unconst_cast(_owner);
    _lights.Init(type, shape, par, type->GetEntityArmor(), type->GetDamageInfo());
    // Total damage
    RStringB damageSel = par >> "selectionDamage";
    _totalDamage.Init(shape, type->_damageInfo, damageSel, NULL);
    // proxy objects
    _proxies.Init(shape);
  }
}

void ClothingType::WearModel::LODShapeUnloaded(LODShape *shape)
{
  _hiddenSelections.Clear();
  _totalDamage.Unload();
  // proxy objects
  _proxies.Deinit(shape);

  if (shape)
  {
    Skeleton *skeleton = shape->GetSkeleton();
    if (skeleton) shape->SkeletonUnassigned(skeleton);
  }
}

void ClothingType::WearModel::ShapeLoaded(LODShape *shape, int level)
{
  if (shape)
  {
    for (int i=0; i<_hiddenSelections.Size(); i++)
      _hiddenSelections[i].InitLevel(shape, level);
    // Lights
    _lights.InitLevel(shape, level);
    // Total damage
    if (_owner->_damageInfo && !_owner->_damageInfo->IsEmpty())
    _totalDamage.InitLevel(shape, level);
    // proxy objects
    _proxies.InitLevel(shape, level);
  }
}

void ClothingType::WearModel::ShapeUnloaded(LODShape *shape, int level)
{
  if (shape)
  {
    for (int i=0; i<_hiddenSelections.Size(); i++)
      _hiddenSelections[i].DeinitLevel(shape, level);
    // Lights
    _lights.DeinitLevel(shape, level);
    // Total damage
    if (_owner->_damageInfo && !_owner->_damageInfo->IsEmpty())
      _totalDamage.DeinitLevel(shape, level);
    _proxies.DeinitLevel(shape, level);
  }
}

Object* ClothingType::CreateObject(bool fullCreate) const
{
  return new Clothing(this, fullCreate);
}

Clothing::Clothing(const EntityAIType *type, bool fullCreate) 
  : base(type, fullCreate)
{
  _clothingType = -1;

  const ClothingType *cType = Type();
  int n = cType->_wearModels.Size();
  _wearModels.Realloc(n); 
  _wearModels.Resize(n); 

  // create lights
  // for (int i=0; i<n; i++) _wearModels[i]._lights.Create(this, cType->_wearModels[i]->_lights);
}

int Clothing::GetProxyCountForLevel(int level) const
{
  if (_clothingType < 0)
    return base::GetProxyCountForLevel(level);
  else
  {
    // use the correct model
    const ClothingType *type = Type();
    if (type && _clothingType < type->_wearModels.Size())
    {
      ClothingType::WearModel *model = type->_wearModels[_clothingType];
      return model->_proxies.GetProxyCountForLevel(model->_model, level);
    }
    return 0; // fallback
  }
}

const ProxyObjectTyped &Clothing::GetProxyForLevel(int level, int i) const
{
  if (_clothingType < 0)
    return base::GetProxyForLevel(level, i);
  else
  {
    // use the correct model
    const ClothingType *type = Type();
    DoAssert(type && _clothingType < type->_wearModels.Size());
    ClothingType::WearModel *model = type->_wearModels[_clothingType];
    return model->_proxies.GetProxyForLevel(model->_model, level, i);
  }
}

SkeletonIndex Clothing::GetProxyBoneForLevel(int level, int i) const
{
  if (_clothingType < 0)
    return base::GetProxyBoneForLevel(level, i);
  else
  {
    // use the correct model
    const ClothingType *type = Type();
    DoAssert(type && _clothingType < type->_wearModels.Size());
    ClothingType::WearModel *model = type->_wearModels[_clothingType];
    return model->_proxies.GetProxyBoneForLevel(model->_model, level, i);
  }
}

int Clothing::GetProxySectionForLevel(int level, int i) const
{
  if (_clothingType < 0)
    return base::GetProxySectionForLevel(level, i);
  else
  {
    // use the correct model
    const ClothingType *type = Type();
    DoAssert(type && _clothingType < type->_wearModels.Size());
    ClothingType::WearModel *model = type->_wearModels[_clothingType];
    return model->_proxies.GetProxySectionForLevel(model->_model, level, i);
  }
}

void Clothing::SetClothingTypeInternal(int value)
{
  if (value == _clothingType) return;

  // unregister old lights
  if (_clothingType == -1) _lights.Clear();
  else _wearModels[_clothingType]._lights.Clear();
  _shape = nullptr;

  // register new lights
  const ClothingType *cType = Type();
  if (cType)
  {
    if (value == -1)
    {
      _shape = cType->GetShape();
      _lights.Create(this, cType->_lights);
    }
    else
    {
      _shape = cType->_wearModels[value]->_model;
      _wearModels[value]._lights.Create(this, cType->_wearModels[value]->_lights);
    }
  }

  _clothingType = value;
}

void Clothing::UpdateShape(Person *owner)
{
  const ClothingType *type = Type();
  if (!type)
  {
    SetClothingTypeInternal(-1);
    return;
  }

  if (owner)
  {
    int sel = -1;
    const EntityAIType *ownerType = owner->GetType();
    if (ownerType)
    {
      sel = ownerType->_clothingType;
      if (sel >= 0 && sel < type->_wearModels.Size())
      {
        SetClothingTypeInternal(sel);
        return; // selected
      }
    }
  }
 
  // default
  SetClothingTypeInternal(-1);
}

int Clothing::SetClothingType(int value)
{
  int oldType = _clothingType;

  const ClothingType *type = Type();
  if (type)
  {
    if (value >= 0 && value < type->_wearModels.Size())
    {
      SetClothingTypeInternal(value);
      return oldType; // selected
    }
    // default / fallback
    SetClothingTypeInternal(-1);
  }

  return oldType;
}

void Clothing::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  if (_clothingType < 0)
  {
    base::Animate(animContext, level, setEngineStuff, parentObject, dist2);
  }
  else
  {
    // animate the correct model
    const ClothingType *type = Type();
    if (type && _clothingType < type->_wearModels.Size())
    {
      ClothingType::WearModel *model = type->_wearModels[_clothingType];
      // hidden selections
      for (int i=0; i<model->_hiddenSelections.Size(); i++)
      {
        if (_hiddenSelectionsTextures[i] || _hiddenSelectionsMaterials[i])
        {
          model->_hiddenSelections[i].Unhide(animContext, _shape, level);
          if (_hiddenSelectionsTextures[i]) model->_hiddenSelections[i].SetTexture(animContext, _shape, level, _hiddenSelectionsTextures[i]);
          if (_hiddenSelectionsMaterials[i]) model->_hiddenSelections[i].SetMaterial(animContext, _shape, level, _hiddenSelectionsMaterials[i]);
        }
        else
          model->_hiddenSelections[i].Hide(animContext, _shape, level);
      }
      // lights
      _wearModels[_clothingType]._lights.Animate(animContext, level, this, model->_lights);
      // total damage
      if (type->_damageInfo && !type->_damageInfo->IsEmpty())
      {
        float dam = GetRawTotalDamage();
        int damLevel = 0;
        if (dam>=1.0)
          damLevel = 2;
        else if (dam>=0.5)
          damLevel = 1;
        if (damLevel>0)
          model->_totalDamage.Apply(animContext, _shape, level, damLevel, dist2);
      }
    }
  }
}

void Clothing::Deanimate(int level, bool setEngineStuff)
{
  if (_clothingType < 0)
    base::Deanimate(level, setEngineStuff);
}

void Clothing::Simulate(float deltaT, SimulationImportance prec)
{
  base::Simulate(deltaT, prec);
}

void Clothing::UpdateLights(const PositionRender &parentTransform)
{
  // called from the parent in the hierarchy
  const ClothingType *type = Type();
  if (type)
  {
    // simulate lights
    ObjectLights *lights = NULL;
    const ObjectTypeLights *typeLights = NULL;
    if (_clothingType < 0)
    {
      lights = &_lights;
      typeLights = &type->_lights;
    }
    else if (_clothingType < _wearModels.Size())
    {
      lights = &_wearModels[_clothingType]._lights;
      typeLights = &type->_wearModels[_clothingType]->_lights;
    }

    if (lights)
      base::UpdateLights(parentTransform, *lights, *typeLights);
  }

}

void Clothing::UpdatePosition()
{
  if (GetHierarchyParent()) return; // handled by the root of the hierarchy

  const ClothingType *type = Type();
  if (type)
  {
    // simulate lights
    ObjectLights *lights = NULL;
    const ObjectTypeLights *typeLights = NULL;
    if (_clothingType < 0)
    {
      lights = &_lights;
      typeLights = &type->_lights;
    }
    else if (_clothingType < _wearModels.Size())
    {
      lights = &_wearModels[_clothingType]._lights;
      typeLights = &type->_wearModels[_clothingType]->_lights;
    }

    if (lights)
    {
      lights->AnimatePosition(this, *typeLights);
      lights->Switch(IsLightOn(FutureVisualState()), this, *typeLights);
      lights->UpdateAggLights(this);
    }
  }
}

