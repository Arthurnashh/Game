#include "../wpch.hpp"
#include "map.hpp"
#include <lib/World.hpp>
#include <lib/Network/network.hpp>
#include <lib/person.hpp>
#include <lib/AI/ai.hpp>

void ItemMapType::Load (ParamEntryPar par, const char *shape)
{
  base::Load(par, shape);
  _mapProperties.Load(par >> "MapProperties");
  _world = par >> "world";
}

Object* ItemMapType::CreateObject (bool fullCreate) const
{
  return new ItemMap(this, fullCreate);
}

bool ItemMap::UseFromInventory (Person *owner)
{
  // UI based action have to be executed on client
  if (GetNetworkManager().IsServer() && !owner->IsLocal())
    GetNetworkManager().UseItem(owner, this);
  else
  {
    const ItemMapType *type = Type();
    if (!type)
      return false;
    if (!type->_world.IsEmpty() && stricmp(type->_world, Glob.header.worldname) != 0)
      return false; // map limited to other world
    if (GWorld)
      GWorld->ShowMapItem(this);
  }
  return true;
}

bool ItemMap::CanAutoShortcut () const
{
  const ItemMapType *type = Type();
  if (!type)
    return false;
  return type->_world.IsEmpty() || stricmp(type->_world, Glob.header.worldname) == 0;
}

