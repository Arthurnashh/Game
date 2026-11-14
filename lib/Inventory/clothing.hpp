#pragma once
#include "inventory.hpp"

extern DynEnum ClothingTypes;

class ClothingType : public InventoryItemType
{
  typedef InventoryItemType base;
  friend class Clothing;

  class WearModel : public LODShapeLoadHandler
  {
    const ClothingType *_owner;

  public:
    Ref<LODShape> _model;
    AutoArray<AnimationSection> _hiddenSelections;
    /// show total damage
    WoundTextureSelections _totalDamage;

    //! Lights grouped in aggregation chunks
    ObjectTypeLights _lights;

    /// container for proxy objects
    ProxiesHolder _proxies;

    WearModel(const ClothingType *owner) {_owner = owner;}

    //@{ LODShapeLoadHandler
    virtual void LODShapeLoaded(LODShape *shape) override;
    virtual void LODShapeUnloaded(LODShape *shape) override;
    virtual void ShapeLoaded(LODShape *shape, int level) override;
    virtual void ShapeUnloaded(LODShape *shape, int level) override;
    //@} LODShapeLoadHandler
  };

  AutoArray<RString> _clothingTypes;
  AutoArray< SRef<WearModel> > _wearModels;

public:
  ClothingType(ParamEntryPar param) : base(param) {}
  virtual void Load(ParamEntryPar par, const char *shape) override;

  virtual void InitShape() override; // force to load shape variants
  virtual void DeinitShape() override;

  virtual Object* CreateObject(bool fullCreate) const override;
};

class Clothing : public InventoryItem
{
  typedef InventoryItem base;

  // who is wearing it currently
  int _clothingType;

  class WearModel
  {
  public:
    //! Each item of the array contains group of aggregated lights
    ObjectLights _lights;

    ClassIsMovable(WearModel);
  };
  AutoArray<WearModel> _wearModels;

public:
  Clothing(const EntityAIType *type, bool fullCreate);

  const ClothingType *Type() const
  {
    return static_cast<const ClothingType *>(GetType());
  }

  virtual void UpdateShape(Person *owner) override;
  virtual int SetClothingType(int value) override;

  virtual void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2) override;
  virtual void Deanimate(int level, bool setEngineStuff) override;

  virtual void Simulate(float deltaT, SimulationImportance prec) override;

  virtual void UpdatePosition() override;
  virtual void UpdateLights(const PositionRender &transform);

  virtual int GetProxyCountForLevel(int level) const;
  virtual const ProxyObjectTyped &GetProxyForLevel(int level, int i) const;
  virtual SkeletonIndex GetProxyBoneForLevel(int level, int i) const;
  virtual int GetProxySectionForLevel(int level, int i) const;

private:
  void SetClothingTypeInternal(int value);
};


