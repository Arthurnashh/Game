#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PROXY_WEAPON_HPP
#define _PROXY_WEAPON_HPP

#include "vehicle.hpp"

#if _ENABLE_NEWHEAD

class ProxySubpartType : public EntityType
{
  typedef EntityType base;
public:
  ProxySubpartType(ParamEntryPar param);
  void Load(ParamEntryPar par, const char *shape);
  void InitShape();
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;
};

class ProxySubpart : public Vehicle
{
  typedef Vehicle base;
public:
  ProxySubpart(const EntityType *type, LODShape *shape);
  const ProxySubpartType *Type() const
  {
    return static_cast<const ProxySubpartType *>(GetNonAIType());
  }
  USE_CASTING(base)
};

#endif

//////////////////////////////////////////////////////////////////////////

// Inventory slots on person
#define INVENTORY_SLOT_ENUM(type, prefix, XX) \
  XX(type, prefix, Shoulder) \
  XX(type, prefix, Melee) \
  XX(type, prefix, Headgear) \
  XX(type, prefix, Mask) \
  XX(type, prefix, Eyewear) \
  XX(type, prefix, Body) \
  XX(type, prefix, Legs) \
  XX(type, prefix, Feet) \
  XX(type, prefix, Vest) \
  XX(type, prefix, Gloves) \
  XX(type, prefix, Back) \
  XX(type, prefix, Hands) /* temporary slot for the item in hands not fitting into inventory */ \
  XX(type, prefix, Shoulder2) /* second large weapon carried across the back */ \

#ifndef DECL_ENUM_INVENTORY_SLOT_ID
#define DECL_ENUM_INVENTORY_SLOT_ID
DECL_ENUM(InventorySlotId)
#endif
DEFINE_ENUM_BEG(InventorySlotId)
  INVENTORY_SLOT_ENUM(InventorySlotId, IS, ENUM_VALUE)
  NInventorySlotId
DEFINE_ENUM_END(type)

extern DynEnum InventorySlotTypes;

class ProxyInventoryType: public EntityType
{
private:
  typedef EntityType base;
  InventorySlotId _slot;

public:
  ProxyInventoryType(ParamEntryPar param) : base(param), _slot((InventorySlotId)-1) {}
  virtual void Load(ParamEntryPar par, const char *shape) override;

  virtual bool AbstractOnly() const override {return false;}
  virtual InventorySlotId GetInventorySlot() const override {return _slot;}
  virtual Object* CreateObject(bool unused) const override { return new ObjectTyped(GetShape(), this, VISITOR_NO_ID, false); }

  // special value for item in hands (not a regular slot)
  static const InventorySlotId _hands = (InventorySlotId)INT_MAX;
};

// CrewPosition enum: see also config: CfgNonAIVehicles
enum CrewPosition {CPDriver,CPGunner,CPCommander,CPCargo};

class ProxyCrewType: public EntityType
{
	typedef EntityType base;
	CrewPosition _pos;

	public:
	ProxyCrewType(ParamEntryPar param);

	void Load(ParamEntryPar par, const char *shape);
	void InitShape();
	bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;

	CrewPosition GetCrewPosition() const {return _pos;}

};


class ProxyCrew: public Entity
{
	typedef Entity base;

	public:
	ProxyCrew(const EntityType *type);

	const ProxyCrewType *Type() const
	{
		return static_cast<const ProxyCrewType *>(GetNonAIType());
	}

	USE_CASTING(base)
};

#endif
