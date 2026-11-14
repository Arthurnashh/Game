#pragma once
#include "inventory.hpp"

class ItemOpticsType : public InventoryItemType
{
  typedef InventoryItemType base;

public:
  /// 2D optics model
  RStringB _oModelName;
  Ref<LODShape> _opticsModel;
  /// opticsInfo
  OpticsInfo _opticInfo;
  /// fire animation for 2D optics
  AnimationAnimatedTexture _animFire;

  ItemOpticsType(ParamEntryPar param) : base(param) {}
  virtual void Load(ParamEntryPar par, const char *shape) override;
  virtual Object* CreateObject(bool fullCreate) const override;

  virtual void InitShape() override;
  virtual void DeinitShape() override;
  //@{ LODShapeLoadHandler
  virtual void LODShapeLoaded(LODShape *shape);
  virtual void LODShapeUnloaded(LODShape *shape);
  virtual void ShapeLoaded(LODShape *shape, int level);
  virtual void ShapeUnloaded(LODShape *shape, int level);
  //@} LODShapeLoadHandler
};

class ItemOptics : public InventoryItem
{
private:
  typedef InventoryItem base;
  // servers only to set the cameraPos through reading attachment configFile only once after attached on muzzle
  const LODShape *_shapeValidatedFor;

public:
  OpticsCameraInfo _opticsCameraInfo;

  ItemOptics(const EntityAIType *type, bool fullCreate) : base(type, fullCreate), _shapeValidatedFor(nullptr) {}

  const ItemOpticsType *Type() const
  {
    return static_cast<const ItemOpticsType *>(GetType());
  }

  ItemOptics *GetOptics() override {return this;}
  const ItemOptics *GetOptics() const override  {return this;}
  // returns true when ItemOptics were already initialized with given shape, false otherwise
  bool ValidateCameraParamsFor(const LODShape *shape)
  {
    if (shape==_shapeValidatedFor) return true;
    _shapeValidatedFor = shape;
    return false;
  }
};

