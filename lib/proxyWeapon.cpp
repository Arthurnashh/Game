#include "wpch.hpp"

#include "vehicle.hpp"
#include <El/ParamFile/paramFile.hpp>

#include "proxyWeapon.hpp"

#if _ENABLE_NEWHEAD

ProxySubpartType::ProxySubpartType(ParamEntryPar param)
:base(param)
{
}

void ProxySubpartType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  // set HW animation type
  // check if the skinning engine is powerfull enough
  if (GEngine->GetMaxBonesCount()>=30)
  {
    _shapeAnimated = AnimTypeHardware;
  }
}

void ProxySubpartType::InitShape()
{
  base::InitShape();
}


Object* ProxySubpartType::CreateObject(bool unused) const
{ 
  return new ProxySubpart(this, GetShape());
}


DEFINE_CASTING(ProxySubpart)

ProxySubpart::ProxySubpart(const EntityType *type, LODShape *shape)
:base(shape, type, CreateObjectId())
{
}

#endif

//////////////////////////////////////////////////////////////////////////

DynEnum InventorySlotTypes;

void ProxyInventoryType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  RStringB slot = par >> "inventorySlot";
  if (stricmp(slot, "hands") == 0) _slot = _hands;
  else
  {
    _slot = (InventorySlotId)-1;
    if (!slot.IsEmpty())
    {
      _slot = (InventorySlotId)InventorySlotTypes.AddValue(slot, false);
      if (_slot == -1)
      {
        RptF("Unknown inventory slot '%s' in class '%s'", cc_cast(slot), cc_cast(par.GetContext()));
      }
    }
  }
}

ProxyCrewType::ProxyCrewType(ParamEntryPar param)
:base(param)
{
}

void ProxyCrewType::Load(ParamEntryPar par, const char *shape)
{
	base::Load(par,shape);
	int pos = par>>"crewPosition";
	_pos = (CrewPosition)pos;

}

void ProxyCrewType::InitShape()
{
	base::InitShape();
}


Object* ProxyCrewType::CreateObject(bool unused) const
{ 
  return new ProxyCrew(this);
}


DEFINE_CASTING(ProxyCrew)

ProxyCrew::ProxyCrew(const EntityType *type)
:base(type->GetShape(),type,CreateObjectId())
{
}

#include <El/Modules/modules.hpp>

#define ENUM_ADD_VALUE(type, typeprefix, name) \
{ int value = InventorySlotTypes.AddValue(#name); DoAssert(value == typeprefix##name); }

INIT_MODULE(ProxyWeapon, 2)
{
  // first values of InventorySlotTypes are reserved for character's slots (InventorySlotId values)
  INVENTORY_SLOT_ENUM(InventorySlotId, IS, ENUM_ADD_VALUE)
}