#pragma once
#include "inventory.hpp"
#include <lib/UI/mapProperties.hpp>

class ItemMapType : public InventoryItemType
{
  typedef InventoryItemType base;
  friend class ItemMap;

  MapProperties _mapProperties;
  RString _world;

public:
  ItemMapType(ParamEntryPar param) : base(param) {}
  virtual void Load(ParamEntryPar par, const char *shape) override;

  virtual Object* CreateObject(bool fullCreate) const override;
  const MapProperties & GetMapProperties() const {return _mapProperties;}
};


class ItemMap : public InventoryItem
{
  typedef InventoryItem base;

public:
  ItemMap(const EntityAIType *type, bool fullCreate)
    : base(type, fullCreate)
  { }

  const ItemMapType * Type() const
  {
    return static_cast<const ItemMapType *>(GetType());
  }

  virtual bool CanAutoShortcut() const override;
  virtual bool UseFromInventory(Person *owner) override;
};


