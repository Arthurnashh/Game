#include "../wpch.hpp"

#if _ENABLE_EDITOR2

// void ExportMission(ArcadeTemplate &t, RString filename) - export from old mission editor

#include "missionEditor.hpp"

#include "../arcadeTemplate.hpp"
#include "../Network/network.hpp"
#include "../soldierOld.hpp"
#include "../invisibleVeh.hpp"

template <class Type>
inline RString ValueToString(const Type &value)
{
  return FindEnumName(value);
}

template <>
inline RString ValueToString(const RString &value)
{
  return value;
}

template <>
inline RString ValueToString(const float &value)
{
  return Format("%.8g", value);
}

template <>
inline RString ValueToString(const int &value)
{
  return Format("%d", value);
}

template <>
inline RString ValueToString(const VisitorObjId &value)
{
  return Format("%d", value.Encode());
}

template <>
inline RString ValueToString(const ObjectId &value)
{
#if _HELISIM_LARGE_OBJECTID
  return Format("%I64d", value.Encode());
#else
  return Format("%d", value.Encode());
#endif // _HELISIM_LARGE_OBJECTID
}

template <>
inline RString ValueToString(const bool &value)
{
  if (value)
    return "true";
  else
    return "false";
}

template <>
inline RString ValueToString(const Vector3 &value)
{
  return Format("[%.8g, %.8g]", value.X(), value.Z());
}

template <>
inline RString ValueToString(const TargetSide &value)
{
  switch (value)
  {
  case TWest:
    return "west";
  case TEast:
    return "east";
  case TCivilian:
    return "civilian";
  case TGuerrila:
    return "resistance";
  case TLogic:
    return "sideLogic";
  default:
    Fail("Side");
    return "ERROR";
  }
}

template <class Type>
void SetObjectArgument(EditorObject *object, RString name, const Type &value)
{
  object->SetArgument(name, ValueToString(value));
};

struct CenterInfo
{
  TargetSide side;
  EditorObject *object;
};
TypeIsMovableZeroed(CenterInfo)

struct MarkerInfo
{
  RString name;
  EditorObject *object;
};
TypeIsMovableZeroed(MarkerInfo)

struct VehicleInfo
{
  int id;
  EditorObject *object;
};
TypeIsMovableZeroed(VehicleInfo)

struct SyncInfo
{
  int sync;
  EditorObject *object1;
  EditorObject *object2;
};
TypeIsMovableZeroed(SyncInfo)

TypeIsSimple(EditorObject *)

// export

static void ExportIntel(EditorWorkspace &ws, const ArcadeIntel &intel)
{
  EditorObjectType *type = ws.FindObjectType("intel");
  if (!type)
  {
    RptF("Missing intel object type");
    return;
  }
  EditorObject *object = ws.AddObject(type);
  SetObjectArgument(object, "OVERCAST", intel.weather);
  SetObjectArgument(object, "OVERCAST_WANTED", intel.weatherForecast);
  SetObjectArgument(object, "FOG", intel.fog);
  SetObjectArgument(object, "FOG_WANTED", intel.fogForecast);
  SetObjectArgument(object, "WIND", intel.wind);
  SetObjectArgument(object, "WIND_WANTED", intel.windForecast);
  SetObjectArgument(object, "YEAR", intel.year);
  SetObjectArgument(object, "MONTH", intel.month);
  SetObjectArgument(object, "DAY", intel.day);
  SetObjectArgument(object, "HOUR", intel.hour);
  SetObjectArgument(object, "MINUTE", intel.minute);
}

static EditorObject *ExportCenter(EditorWorkspace &ws, TargetSide side, const ArcadeIntel &intel)
{
  EditorObjectType *type = ws.FindObjectType("center");
  if (!type)
  {
    RptF("Missing center object type");
    return NULL;
  }
  EditorObject *object = ws.AddObject(type);
  SetObjectArgument(object, "SIDE", side);
  if (side == TLogic)
  {
    SetObjectArgument(object, "FRIEND_EAST", 1.0f);
    SetObjectArgument(object, "FRIEND_WEST", 1.0f);
    SetObjectArgument(object, "FRIEND_RESISTANCE", 1.0f);
  }
  else if (side == TCivilian)
  {
    SetObjectArgument(object, "FRIEND_EAST", intel.friends[TGuerrila][TEast]);
    SetObjectArgument(object, "FRIEND_WEST", intel.friends[TGuerrila][TWest]);
    SetObjectArgument(object, "FRIEND_RESISTANCE", intel.friends[TGuerrila][TGuerrila]);
  }
  else
  {
    SetObjectArgument(object, "FRIEND_EAST", intel.friends[side][TEast]);
    SetObjectArgument(object, "FRIEND_WEST", intel.friends[side][TWest]);
    SetObjectArgument(object, "FRIEND_RESISTANCE", intel.friends[side][TGuerrila]);
  }
  SetObjectArgument(object, "FRIEND_CIVILIAN", 1.0f);
  return object;
}

static EditorObject *ExportMarker(EditorWorkspace &ws, const ArcadeMarkerInfo &marker)
{
  EditorObjectType *type = ws.FindObjectType("marker");
  if (!type)
  {
    RptF("Missing marker object type");
    return NULL;
  }
  EditorObject *object = ws.AddObject(type);
  SetObjectArgument(object, "POSITION", marker.position);
  SetObjectArgument(object, "NAME", marker.name);
  SetObjectArgument(object, "TEXT", marker.text);
  SetObjectArgument(object, "MARKER_TYPE", marker.markerType);
  SetObjectArgument(object, "TYPE", marker.type);
  SetObjectArgument(object, "COLOR", marker.colorName);
  SetObjectArgument(object, "FILL", marker.fillName);
  float size = 1.0f;
  if (marker.markerType == MTIcon)
  {
    size = marker.size;
    const float defaultMarkerSize = 32.0f;
    if (size == 0) size = defaultMarkerSize;
  }
  
  SetObjectArgument(object, "A", marker.a * size);
  SetObjectArgument(object, "B", marker.b * size);
  SetObjectArgument(object, "ANGLE", marker.angle);
  SetObjectArgument(object, "DRAWBORDER", marker.drawBorder);
  return object;
}

RString CreateCondition(RString presenceCondition, float presence)
{
  if (presence < 1)
  {
    if (strcmp(presenceCondition, "true") == 0) return Format("%.8g > random 1", presence);
    return Format("(%s) and (%.8g > random 1)", (const char *)presenceCondition, presence);
  }
  else return presenceCondition;
}

static EditorObject *ExportVehicle(EditorWorkspace &ws, const ArcadeUnitInfo &unit, AutoArray<MarkerInfo> &markerMap)
{
  RString vehClass = Pars >> "CfgVehicles" >> unit.vehicle >> "vehicleClass";

  bool vehicle = false;
  EditorObjectType *type = NULL;

  if (stricmp(vehClass, "Sounds") == 0)
  {
    // sound source
    type = ws.FindObjectType("soundSource");
    if (!type)
    {
      RptF("Missing soundSource object type");
      return NULL;
    }
  }
  else if (stricmp(vehClass, "Mines") == 0)
  {
    // mine
    type = ws.FindObjectType("mine");
    if (!type)
    {
      RptF("Missing mine object type");
      return NULL;
    }
  }
  else
  {
    // regular vehicle
    type = ws.FindObjectType("vehicle");
    if (!type)
    {
      RptF("Missing vehicle object type");
      return NULL;
    }
    vehicle = true;
  }

  EditorObject *object = ws.AddObject(type);

  RString condition = CreateCondition(unit.presenceCondition, unit.presence);
  SetObjectArgument(object, "PRESENCE_CONDITION", condition);
  SetObjectArgument(object, "POSITION", unit.position);
  SetObjectArgument(object, "TYPE", unit.vehicle);
  SetObjectArgument(object, "PLACEMENT", unit.placement);
  if (!vehicle) return NULL; // return object only for vehicles

  SetObjectArgument(object, "SPECIAL", unit.special);
  SetObjectArgument(object, "AZIMUT", unit.azimut);
  SetObjectArgument(object, "NAME", unit.name);
  SetObjectArgument(object, "HEALTH", unit.health);
  SetObjectArgument(object, "FUEL", unit.fuel);
  SetObjectArgument(object, "AMMO", unit.ammo);
  SetObjectArgument(object, "ID", unit.id);
  SetObjectArgument(object, "AGE", unit.age);
  SetObjectArgument(object, "LOCK", unit.lock);
  SetObjectArgument(object, "INIT", unit.init);
  return object;  
}

static void ExportCrew
(
  EditorWorkspace &ws, const ArcadeUnitInfo &unit, EditorObject *group, EditorObject *vehicle,
  RString crewType, Rank rank, GetInPosition pos, bool player, bool leader
)
{
  Assert(group);
  Assert(vehicle);

  // soldier in vehicle
  EditorObjectType *type = ws.FindObjectType("unit");
  if (!type)
  {
    RptF("Missing unit object type");
    return;
  }
  EditorObject *object = ws.AddObject(type);

  RString grpName = group->GetArgument("VARIABLE_NAME");
  RString vehName = vehicle->GetArgument("VARIABLE_NAME");

  SetObjectArgument(object, "PRESENCE_CONDITION", Format("not isNull %s", (const char *)vehName));
  SetObjectArgument(object, "POSITION", unit.position);
  SetObjectArgument(object, "GROUP", grpName);
  SetObjectArgument(object, "TYPE", crewType);
  SetObjectArgument(object, "PLACEMENT", 0);
  SetObjectArgument(object, "SPECIAL", ASpNone);
  SetObjectArgument(object, "AZIMUT", 0);
  SetObjectArgument(object, "HEALTH", 1);
  SetObjectArgument(object, "AMMO", 1);
  SetObjectArgument(object, "ID", -1);
  SetObjectArgument(object, "AGE", AAUnknown);
  SetObjectArgument(object, "INIT", RString());
  SetObjectArgument(object, "RANK", rank);
  SetObjectArgument(object, "SKILL", unit.skill);
  RString name;
  switch (pos)
  {
  case GIPCommander:
    SetObjectArgument(object, "COMMANDER", vehName);
    if (unit.name.GetLength() > 0) name = unit.name + RString("c");
    break;
  case GIPDriver:
    SetObjectArgument(object, "DRIVER", vehName);
    if (unit.name.GetLength() > 0) name = unit.name + RString("d");
    break;
  case GIPGunner:
    SetObjectArgument(object, "GUNNER", vehName);
    if (unit.name.GetLength() > 0) name = unit.name + RString("g");
    break;
  }
  SetObjectArgument(object, "NAME", name);
  SetObjectArgument(object, "PLAYER", player);
  if (leader) SetObjectArgument(object, "LEADER", grpName);
}

static EditorObject *ExportUnit(EditorWorkspace &ws, const ArcadeUnitInfo &unit, EditorObject *group, AutoArray<MarkerInfo> &markerMap)
{
  // free soldier / soldier in cargo
  Assert(group);
  //Assert(vehicle);

  // soldier in vehicle
  EditorObjectType *type = ws.FindObjectType("unit");
  if (!type)
  {
    RptF("Missing unit object type");
    return NULL;
  }
  EditorObject *object = ws.AddObject(type);

  RString grpName = group->GetArgument("VARIABLE_NAME");

  RString condition = CreateCondition(unit.presenceCondition, unit.presence);
  SetObjectArgument(object, "PRESENCE_CONDITION", condition);
  SetObjectArgument(object, "POSITION", unit.position);
  SetObjectArgument(object, "GROUP", grpName);
  SetObjectArgument(object, "TYPE", unit.vehicle);
  SetObjectArgument(object, "PLACEMENT", unit.placement);
  SetObjectArgument(object, "SPECIAL", unit.special);

  SetObjectArgument(object, "AZIMUT", unit.azimut);
  SetObjectArgument(object, "NAME", unit.name);
  SetObjectArgument(object, "HEALTH", unit.health);
  SetObjectArgument(object, "AMMO", unit.ammo);
  SetObjectArgument(object, "ID", unit.id);
  SetObjectArgument(object, "AGE", unit.age);
  SetObjectArgument(object, "INIT", unit.init);
  SetObjectArgument(object, "RANK", unit.rank);
  SetObjectArgument(object, "SKILL", unit.skill);

  SetObjectArgument(object, "PLAYER", unit.player == APPlayerCommander);
  if (unit.leader) SetObjectArgument(object, "LEADER", grpName);

  return object;
}

static void ExportEffects(EditorObject *object, const ArcadeEffects &effects)
{
  SetObjectArgument(object, "EFFECT_CONDITION", effects.condition);
  SetObjectArgument(object, "SOUND_EFFECT", effects.sound);
  SetObjectArgument(object, "VOICE_EFFECT", effects.voice);
  SetObjectArgument(object, "SOUND_ENV_EFFECT", effects.soundEnv);
  SetObjectArgument(object, "SOUND_DET_EFFECT", effects.soundDet);
  SetObjectArgument(object, "MUSIC_EFFECT", effects.track);
  SetObjectArgument(object, "TITLE_EFFECT_TYPE", effects.titleType);
  SetObjectArgument(object, "TITLE_EFFECT_EFFECT", effects.titleEffect);
  SetObjectArgument(object, "TITLE_EFFECT_TITLE", effects.title);
}


void ExportMission(ArcadeTemplate &t, RString filename)
{
  EditorWorkspace ws;

  // addons
  EditorObject *object = ws.GetPrefixObject();
  for (int i=0; i<t.addOns.Size(); i++) SetObjectArgument(object, "ADDON", t.addOns[i]);
  
  // helper structures for fast searching
  AutoArray<CenterInfo> centerMap;
  AutoArray<MarkerInfo> markerMap;
  AutoArray<VehicleInfo> vehicleMap;
  AutoArray<EditorObject *> groupMap;

  // intel
  ExportIntel(ws, t.intel);

  // markers
  for (int i=0; i<t.markers.Size(); i++)
  {
    int index = markerMap.Add();
    markerMap[index].name = t.markers[i].name;
    markerMap[index].object = ExportMarker(ws, t.markers[i]);
  }

  // empty vehicles
  for (int i=0; i<t.emptyVehicles.Size(); i++)
  {
    const ArcadeUnitInfo &unit = t.emptyVehicles[i];
    EditorObject *vehicle = ExportVehicle(ws, unit, markerMap);
    Assert(vehicle);
    if (unit.id >= 0)
    {
      int index = vehicleMap.Add();
      vehicleMap[index].id = unit.id;
      vehicleMap[index].object = vehicle;
    }
  }

  ws.WriteCreateScript(filename + RString(".sqf"));
  ws.Save(filename + RString(".biedi"));
}

#endif // _ENABLE_EDITOR2