#ifndef _UNITS_ON_LAND_HPP
#define _UNITS_ON_LAND_HPP

#include "person.hpp"
#include "landscape.hpp"

template<class ObjectArrayType>
class AddVehCrewFunc : public ICrewFunc
{
protected:
  ObjectArrayType &_units;
  float _maxDist;
  Vector3 _pos;

public:
  AddVehCrewFunc(ObjectArrayType &units, Vector3Par pos, float maxDist) : _units(units), _pos(pos), _maxDist(maxDist)
  {}

  virtual bool operator () (Person *person)
  {
    AIBrain *brain = person->Brain();
    if (!brain || (brain->GetLifeState() == LifeStateDead) ) return false;
    if (!person->IsRemotePlayer()) return false;
    float dist2 = _pos.Distance2(person->WorldPosition(person->FutureVisualState()));
    if (dist2 <= Square(_maxDist)) _units.Add(person);
    return false;
  }
};

/*!
\patch 5209 Date 1/3/2008 by Bebul
- Fixed: Direct speech inside vehicles was often inaudible.
*/

template<class ObjectArrayType>
void WhatUnitsDirect(ObjectArrayType &units, Person *person, float maxDist=40.0f)
{
  Vector3Val pos = person->WorldPosition(person->FutureVisualState());
  int xMin, xMax, zMin, zMax;
  ObjRadiusRectangle(xMin, xMax, zMin, zMax, pos, pos, maxDist);
#if VOICE_OVER_NET
  // get the list of dpnid
  AutoArray<int> unitsDpnid;
  void WhatUnitsVoNDirect(AutoArray<int> &units, Vector3Par pos, float maxDist);
  WhatUnitsVoNDirect(unitsDpnid, pos, maxDist);
  // Get All Players
  typedef OLink(Person) helperType;
  AUTO_STATIC_ARRAY(helperType, players, 128);
  GWorld->ScanPlayers(players);

  for (int i=0; i<unitsDpnid.Size(); i++)
  {
    for (int j=0; j<players.Size(); j++)
    {
      if (players[j]->GetRemotePlayer()==unitsDpnid[i])
      {
        Person *person = players[j];
        units.Add(person);
        break;
      }
    }
  }
#else
  for (int x=xMin; x<=xMax; x++)
    for (int z=zMin; z<=zMax; z++)
    {
      if (!InRange(x, z)) continue;

      const ObjectList &list = GLandscape->GetObjects(z, x);
      int n = list.Size();
      for (int i=0; i<n; i++)
      {
        Object *obj = list[i];
        if (!obj) continue;
        if (obj->GetType() != TypeVehicle) continue;

        EntityAI *veh = dyn_cast<EntityAI>(obj);
        if (!veh) continue;
        if (veh->IsDamageDestroyed()) continue;

        AddVehCrewFunc<ObjectArrayType> addCrewFunc(units,pos,maxDist);
        veh->ForEachCrew(addCrewFunc);
      }
    } // (for all near objects)
#endif
}

#endif

