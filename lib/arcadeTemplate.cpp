#include "wpch.hpp"
#include "Network/networkImpl.hpp"

#include "arcadeTemplate.hpp"
#include "AI/ai.hpp"
#include <El/ParamFile/paramFile.hpp>
#include "paramArchiveExt.hpp"
#include "landscape.hpp"
#include "global.hpp"
#include <El/Common/randomGen.hpp>

#include "UI/resincl.hpp"
#include "UI/uiControls.hpp"
#include "world.hpp"
#include "fileLocator.hpp"

#include <El/Enum/enumNames.hpp>

#include "stringtableExt.hpp"
#include "UI/chat.hpp"


RString GetVehicleIcon(RString name);

static const EnumName TitleTypeNames[]=
{
  EnumName(TitleNone, "NONE", &IDS_TITTYPE_NONE),
  EnumName(TitleObject, "OBJECT", &IDS_TITTYPE_OBJECT),
  EnumName(TitleResource, "RES", &IDS_TITTYPE_RESOURCE),
  EnumName(TitleText, "TEXT", &IDS_TITTYPE_TEXT),
  EnumName()
};
template<>
const EnumName *GetEnumNames(TitleType dummy)
{
  return TitleTypeNames;
}

static const EnumName CombatModeNames[]=
{
  EnumName(CMUnchanged, "UNCHANGED", &IDS_COMBAT_UNCHANGED),
  EnumName(CMCareless, "CARELESS", &IDS_COMBAT_CARELESS),
  EnumName(CMSafe, "SAFE", &IDS_COMBAT_SAFE),
  EnumName(CMAware, "AWARE", &IDS_COMBAT_AWARE),
  EnumName(CMCombat, "COMBAT", &IDS_COMBAT_COMBAT),
  EnumName(CMStealth, "STEALTH", &IDS_COMBAT_STEALTH),
  EnumName()
};
template<>
const EnumName *GetEnumNames(CombatMode dummy)
{
  return CombatModeNames;
}

static const EnumName ArcadeUnitSpecialNames[]=
{
  EnumName(ASpNone, "NONE", &IDS_SPECIAL_NONE),
  EnumName(ASpCargo, "CARGO", &IDS_SPECIAL_CARGO),
  EnumName(ASpFlying, "FLY", &IDS_SPECIAL_FLYING),
  EnumName(ASpForm, "FORM", &IDS_SPECIAL_FORM),
  EnumName(ASpCanCollide, "CAN_COLLIDE", &IDS_SPECIAL_CAN_COLLIDE),
  EnumName()
};
template<>
const EnumName *GetEnumNames(ArcadeUnitSpecial dummy)
{
  return ArcadeUnitSpecialNames;
}

static const EnumName ArcadeUnitAgeNames[]=
{
  EnumName(AAActual, "ACTUAL", &IDS_AGE_ACTUAL),
  EnumName(AA5Min, "5 MIN", &IDS_AGE_5MIN),
  EnumName(AA10Min, "10 MIN", &IDS_AGE_10MIN),
  EnumName(AA15Min, "15 MIN", &IDS_AGE_15MIN),
  EnumName(AA30Min, "30 MIN", &IDS_AGE_30MIN),
  EnumName(AA60Min, "60 MIN", &IDS_AGE_60MIN),
  EnumName(AA120Min, "120 MIN", &IDS_AGE_120MIN),
  EnumName(AAUnknown, "UNKNOWN", &IDS_AGE_UNKNOWN),
  EnumName()
};
template<>
const EnumName *GetEnumNames(ArcadeUnitAge dummy)
{
  return ArcadeUnitAgeNames;
}

static const EnumName ArcadeUnitPlayerNames[]=
{
  EnumName(APNonplayable, "NONPLAY"),
  EnumName(APPlayerCommander, "PLAYER COMMANDER"),
  EnumName(APPlayerDriver, "PLAYER DRIVER"),
  EnumName(APPlayerGunner, "PLAYER GUNNER"),
  EnumName(APPlayableC, "PLAY C"),
  EnumName(APPlayableD, "PLAY D"),
  EnumName(APPlayableG, "PLAY G"),
  EnumName(APPlayableCD, "PLAY CD"),
  EnumName(APPlayableCG, "PLAY CG"),
  EnumName(APPlayableDG, "PLAY DG"),
  EnumName(APPlayableCDG, "PLAY CDG"),
  // old versions
  EnumName(APPlayableCDG, "PLAY"),
  EnumName(APPlayerCommander, "P1 COMMANDER"),
  EnumName(APPlayerDriver, "P1 DRIVER"),
  EnumName(APPlayerGunner, "P1 GUNNER"),
  EnumName(APPlayerCommander, "P2 COMMANDER"),
  EnumName(APPlayerDriver, "P2 DRIVER"),
  EnumName(APPlayerGunner,"P2 GUNNER"),
  EnumName(APNonplayable, "-1"),
  EnumName(APPlayableCDG, "0"),
  EnumName(APPlayerCommander, "1"),
  EnumName(APPlayerCommander, "2"),
  EnumName(APNonplayable, "-1.000000"),
  EnumName(APPlayableCDG, "0.000000"),
  EnumName(APPlayerCommander, "1.000000"),
  EnumName(APPlayerCommander, "2.000000"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(ArcadeUnitPlayer dummy)
{
  return ArcadeUnitPlayerNames;
}

static const EnumName LockStateNames[]=
{
  EnumName(LSUnlocked, "UNLOCKED", &IDS_VEHICLE_UNLOCKED),
  EnumName(LSDefault, "DEFAULT", &IDS_VEHICLE_DEFAULT),
  EnumName(LSLocked, "LOCKED", &IDS_VEHICLE_LOCKED),
  EnumName(LSLockedForPlayer, "LOCKEDPLAYER", &IDS_VEHICLE_LOCKED_FOR_PLAYER),
  EnumName()
};
template<>
const EnumName *GetEnumNames(LockState dummy)
{
  return LockStateNames;
}

static const EnumName MarkerTypeNames[]=
{
  EnumName(MTIcon, "ICON", &IDS_DISP_ARCMARK_ICON),
  EnumName(MTRectangle, "RECTANGLE", &IDS_DISP_ARCMARK_RECT),
  EnumName(MTEllipse, "ELLIPSE", &IDS_DISP_ARCMARK_ELLIPSE),
  EnumName()
};
template<>
const EnumName *GetEnumNames(MarkerType dummy)
{
  return MarkerTypeNames;
}

///////////////////////////////////////////////////////////////////////////////
// struct ArcadeUnitInfo

const float MinAbility=0.2; // private
const float MinRank=RankPrivate;
const float MaxAbility=1;
const float MaxRank=RankColonel;

float RankToSkill(int rank)
{
  float factor = (MaxAbility - MinAbility) / (MaxRank - MinRank);
  return (rank - MinRank) * factor + MinAbility;
}

ArcadeUnitInfo::ArcadeUnitInfo()
{
  Init();
}

ArcadeUnitInfo::ArcadeUnitInfo(const ArcadeUnitInfo &src)
{
  presence = src.presence;
  presenceCondition = src.presenceCondition;
  position = src.position;
#if _VBS3 // mission.sqm positionASL
  positionASL = src.positionASL;
#endif
  placement = src.placement;
  azimut = src.azimut;
  special = src.special;
  age = src.age;
  id = src.id;
  side = src.side;
  vehicle = src.vehicle;
  icon = src.icon;
  size = src.size;
  player = src.player;
  forceInServer = src.forceInServer;
  leader = src.leader;
  lock = src.lock;
  rank = src.rank;
  skill = src.skill;
  health = src.health;
  fuel = src.fuel;
  ammo = src.ammo;
  name = src.name;
  selected = src.selected;
  init = src.init;
  description = src.description;
  module = src.module;
}

/*!
\patch 1.30 Date 11/03/2001 by Jirka
- Changed: Default skill of units inserted in editor is now half skilled,
not minimum skill as before.
*/

void ArcadeUnitInfo::Init()
{
  presence = 1.0;
  presenceCondition = "true";
  position = VZero;
#if _VBS3 // mission.sqm positionASL
  positionASL = VZero;
#endif
  placement = 0;
  azimut = 0;
  special = ASpForm;
  age = AAUnknown;
  id = 0;
  side = TWest;
  vehicle = "";
  type = NULL;
  icon = NULL;
  size = 0;
  player = APNonplayable;
  forceInServer = false;
  leader = 0;
  lock = LSDefault;
  rank = RankPrivate;
  skill = (MaxAbility + MinAbility)*0.5f;
  health = 1.0;
  fuel = 1.0;
  ammo = 1.0;
  name = "";
  selected = false;
  init = "";
  description = "";
  module = false;
}

void ArcadeUnitInfo::AddOffset(Vector3Par offset)
{
  position += offset;
  position[1] = GLOB_LAND->RoadSurfaceYAboveWater
  (
    position[0], position[2]
  );
}

void ArcadeUnitInfo::Rotate(Vector3Par center, float angle, bool sel)
{
  if (sel && !selected) return;

  // rotation
  azimut += (180.0 / H_PI) * angle;

  Vector3 dir = position - center;
  Matrix3 rot(MRotationY, -angle);
  dir = rot * dir;

  position = center + dir;
  position[1] = GLandscape->RoadSurfaceYAboveWater
  (
    position[0], position[2]
  );
}

void ArcadeUnitInfo::CalculateCenter(Vector3 &sum, int &count, bool sel)
{
  if (sel && !selected) return;
  
  sum += position;
  count++;
}

/*!
\patch 1.63 Date 6/2/2002 by Ondra
- Changed: Addon requirements are now determined by addon entry creator,
not only by CfgPatches config section. This makes addon list more reliable
with addons that ommited to list some added types.
*/

// ADDED in patch 1.01 - AddOns check
void ArcadeUnitInfo::RequiredAddons(FindArrayRStringCI &addOns)
{
  ConstParamEntryPtr patches = Pars.FindEntry("CfgPatches");
  if (!patches) return;
  for (int i=0; i<patches->GetEntryCount(); i++)
  {
    ParamEntryVal patch = patches->GetEntry(i);
    if (!patch.IsClass()) continue;
    for (int j=0; j<(patch >> "units").GetSize(); j++)
    {
      RStringB patchVehicle = (patch >> "units")[j];
      if (stricmp(patchVehicle, vehicle) == 0)
      {
        addOns.AddUnique(patch.GetName());
        goto Break;
      }
    }
  }
  Break:
  // more robust check - check owner of given unit type
  ConstParamEntryPtr entry = (Pars>>"CfgVehicles").FindEntry(vehicle);
  if (entry)
  {
    const RStringB &owner = entry->GetOwnerName();
    if (owner.GetLength()>0)
    {
      addOns.AddUnique(owner);
    }
  }
}

LSError ArcadeUnitInfo::Serialize(ParamArchive &ar)
{
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst) Init();

  CHECK(ar.Serialize("presence", presence, 1, 1.0)) 
  CHECK(ar.Serialize("presenceCondition", presenceCondition, 1, "true"))  
  CHECK(::Serialize(ar, "position", position, 1)) 
  CHECK(ar.Serialize("placement", placement, 1, 0)) 
  CHECK(ar.Serialize("azimut", azimut, 1, 0)) 

#if _VBS3 // mission.sqm positionASL
  if (ar.IsSaving() || ar.GetArVersion() >= 12)
    CHECK(::Serialize(ar, "positionASL", positionASL, 1))
#endif

  CHECK(ar.SerializeEnum("special", special, 1, ASpForm)) 
  CHECK(ar.SerializeEnum("age", age, 1, AAUnknown)) 
  CHECK(ar.Serialize("id", id, 1))  
  CHECK(ar.SerializeEnum("side", side, 1))  

  CHECK(ar.Serialize("vehicle", vehicle, 1))  
  CHECK(ar.SerializeEnum("player", player, 1, APNonplayable)) 
  CHECK(ar.Serialize("forceInServer", forceInServer, 1, false)) 
  CHECK(ar.Serialize("leader", leader, 1, false)) 
  if (ar.IsSaving() || ar.GetArVersion() >= 11)
    CHECK(ar.SerializeEnum("lock", lock, 1, ENUM_CAST(LockState,LSDefault)))  
  else
  {
    bool locked;
    CHECK(ar.Serialize("locked", locked, 7, false)) 
      lock = locked ? LSLocked : LSDefault;
  }
  CHECK(ar.SerializeEnum("rank", rank, 1, RankPrivate)) 
  CHECK(ar.Serialize("skill", skill, 1, -1.0))  
  CHECK(ar.Serialize("health", health, 1, 1.0)) 
  CHECK(ar.Serialize("fuel", fuel, 1, 1.0)) 
  CHECK(ar.Serialize("ammo", ammo, 1, 1.0)) 
  CHECK(ar.Serialize("text", name, 7, ""))  
  CHECK(ar.Serialize("init", init, 7, ""))  
  CHECK(ar.Serialize("description", description, 1, ""))  


  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    ConstParamEntryPtr entry = (Pars >> "CfgVehicles").FindEntry(vehicle);
    if (entry)
    {
      RString iconName = Pars >> "CfgVehicles" >> vehicle >> "icon";
      RString vehicleClass = Pars >> "CfgVehicles" >> vehicle >> "vehicleClass";

      if(strcmpi(vehicleClass.Data(),"Modules")==0) module = true;

      icon = GlobLoadTexture(GetVehicleIcon(iconName));
      size = Pars >> "CfgVehicles" >> vehicle >> "mapSize";
    }
    else
    {
      icon = NULL;
      size = 1;
      WarningMessage("%s: Vehicle class %s no longer exists",cc_cast(ar.GetContext("vehicle")),cc_cast(vehicle));
    }

    ATSParams *params = (ATSParams *)ar.GetParams();
    Assert(params);
    Assert(id >= 0);
//    id += params->baseVeh;
    saturateMax(params->nextVehId, id + 1);
//    if (params->merge) player = APNonplayable;

    if (skill < 0) skill = RankToSkill(rank);
  }

  return LSOK;
}


///////////////////////////////////////////////////////////////////////////////
// struct ArcadeMarkerInfo

ArcadeMarkerInfo::ArcadeMarkerInfo()
{
  Init();
}

ArcadeMarkerInfo::ArcadeMarkerInfo(const ArcadeMarkerInfo &src)
{
  position = src.position;
  name = src.name;
  text = src.text;
  markerType = src.markerType;
  type = src.type;
  colorName = src.colorName;
  color = src.color;
  alpha = src.alpha;
  fillName = src.fillName;
  fill = src.fill;
  icon = src.icon;
  size = src.size;
  a = src.a;
  b = src.b;
  angle = src.angle;
  selected = src.selected;
  drawBorder = src.drawBorder;
  shadow = src.shadow;
#if _VBS2 // conditional markers
  autosize = src.autosize;
  attachCondition = src.attachCondition;
  attachTo = src.attachTo;
  attachCtx = src.attachCtx;
  attachOffset = src.attachOffset;
  condition = src.condition;
  isHidden = src.isHidden;
#endif
}

void ArcadeMarkerInfo::Init()
{
  position = VZero;
  name = "";
  text = "";
  markerType = MTIcon;
  type = "";
  colorName = "Default";
  color = PackedBlack;
  alpha = 1.0f;
  fillName = "Solid";
  fill = NULL;
  icon = NULL;
  size = 24;
  a = 1;
  b = 1;
  angle = 0;
  selected = false;
  drawBorder = false;
#if _VBS2 // conditional markers
  autosize = true;
  attachCondition = "";
  attachTo = NULL;
  attachOffset = VZero;
  condition = "";  
  isHidden = false;
#endif
}

void ArcadeMarkerInfo::AddOffset(Vector3Par offset)
{
  position += offset;
  position[1] = GLOB_LAND->RoadSurfaceYAboveWater
  (
    position[0], position[2]
  );
}

void ArcadeMarkerInfo::Rotate(Vector3Par center, float alpha, bool sel)
{
  if (sel && !selected) return;

  // rotation
  angle += (180.0 / H_PI) * alpha;

  Vector3 dir = position - center;
  Matrix3 rot(MRotationY, -alpha);
  dir = rot * dir;

  position = center + dir;
  position[1] = GLandscape->RoadSurfaceYAboveWater
  (
    position[0], position[2]
  );
}

void ArcadeMarkerInfo::CalculateCenter(Vector3 &sum, int &count, bool sel)
{
  if (sel && !selected) return;
  
  sum += position;
  count++;
}

LSError ArcadeMarkerInfo::Serialize(ParamArchive &ar)
{
  CHECK(::Serialize(ar, "position", position, 1)) 
  CHECK(ar.Serialize("name", name, 1))  
  CHECK(ar.Serialize("text", text, 1, ""))  
  CHECK(ar.SerializeEnum("markerType", markerType, 1, MTIcon))
  CHECK(ar.Serialize("type", type, 1))  
  CHECK(ar.Serialize("colorName", colorName, 1, "Default")) 
  CHECK(ar.Serialize("alpha", alpha, 1, 1.0f))
  CHECK(ar.Serialize("fillName", fillName, 1, "Solid")) 
  CHECK(ar.Serialize("a", a, 1, 1.0f))  
  CHECK(ar.Serialize("b", b, 1, 1.0f))  
  CHECK(ar.Serialize("angle", angle, 1, 0.0f))  
  CHECK(ar.Serialize("drawBorder", drawBorder, 1, false))  
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    OnTypeChanged();
    OnColorChanged();
    OnFillChanged();
  }

  return LSOK;
}

DEFINE_NETWORK_OBJECT_SIMPLE(ArcadeMarkerInfo, Marker)

#if _VBS2 // conditional markers
#define MARKER_MSG(MessageName, XX) \
  XX(MessageName, Vector3, position, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Marker position"), TRANSF) \
  XX(MessageName, RString, name, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Marker (unique) name"), TRANSF) \
  XX(MessageName, RString, text, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Marker title"), TRANSF) \
  XX(MessageName, int, markerType, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, MTIcon), DOC_MSG("Marker type (icon, rectangle, ellipse)"), TRANSF) \
  XX(MessageName, RString, type, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Marker icon"), TRANSF) \
  XX(MessageName, RString, colorName, NDTString, RString, NCTNone, DEFVALUE(RString, "Default"), DOC_MSG("Marker color name"), TRANSF) \
  XX(MessageName, RString, fillName, NDTString, RString, NCTNone, DEFVALUE(RString, "Solid"), DOC_MSG("Marker fill name"), TRANSF) \
  XX(MessageName, float, a, NDTFloat, float, NCTNone, DEFVALUE(float, 1.0f), DOC_MSG("Width"), TRANSF) \
  XX(MessageName, float, b, NDTFloat, float, NCTNone, DEFVALUE(float, 1.0f), DOC_MSG("Height"), TRANSF) \
  XX(MessageName, float, angle, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Rotation"), TRANSF) \
  XX(MessageName, RString, attachCondition, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Marker attach condition"), TRANSF) \
  XX(MessageName, Vector3, attachOffset, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Marker attach offset"), TRANSF) \
  XX(MessageName, OLink(Object), attachTo, NDTRef, NetworkId,  NCTNone, DEFVALUENULL, DOC_MSG("Vehicle to attach marker to"), TRANSF_REF) \
  XX(MessageName, OLinkArray(Object), attachCtx, NDTRefArray, AutoArray<NetworkId>, NCTNone, DEFVALUEREFARRAY, DOC_MSG("Objects to pass into condition as _this"), TRANSF_REFS) \
  XX(MessageName, RString, condition, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Marker visibility condition"), TRANSF) \
  XX(MessageName, bool, isHidden, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Marker is hidden (invisible)"), TRANSF) \
  XX(MessageName, AutoArray<RString>, layeredIcons, NDTStringArray, AutoArray<RString>, NCTNone, DEFVALUESTRINGARRAY, DOC_MSG("List of icons to layer when drawing marker"), TRANSF) \
  XX(MessageName, AutoArray<float>, layeredIconsSizeA, NDTFloatArray, AutoArray<float>, NCTNone, DEFVALUEFLOATARRAY, DOC_MSG("Layered icons size A"), TRANSF) \
  XX(MessageName, AutoArray<float>, layeredIconsSizeB, NDTFloatArray, AutoArray<float>, NCTNone, DEFVALUEFLOATARRAY, DOC_MSG("Layered icons size B"), TRANSF) \
  XX(MessageName, AutoArray<float>, layeredIconsOffsetX, NDTFloatArray, AutoArray<float>, NCTNone, DEFVALUEFLOATARRAY, DOC_MSG("Layered icons offset X"), TRANSF) \
  XX(MessageName, AutoArray<float>, layeredIconsOffsetY, NDTFloatArray, AutoArray<float>, NCTNone, DEFVALUEFLOATARRAY, DOC_MSG("Layered icons offset Y"), TRANSF) \
  XX(MessageName, bool, autosize, NDTBool, bool, NCTNone, DEFVALUE(bool, true), DOC_MSG("Marker will maintain the same size regardless of map zoom"), TRANSF) \
  XX(MessageName, float, alpha, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 1.0f), DOC_MSG("Marker fading"), TRANSF) \
  XX(MessageName, bool, drawBorder, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Draw Border"), TRANSF)
#else
#define MARKER_MSG(MessageName, XX) \
  XX(MessageName, Vector3, position, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Marker position"), TRANSF) \
  XX(MessageName, RString, name, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Marker (unique) name"), TRANSF) \
  XX(MessageName, RString, text, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Marker title"), TRANSF) \
  XX(MessageName, int, markerType, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, MTIcon), DOC_MSG("Marker type (icon, rectangle, ellipse)"), TRANSF) \
  XX(MessageName, RString, type, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Marker icon"), TRANSF) \
  XX(MessageName, RString, colorName, NDTString, RString, NCTNone, DEFVALUE(RString, "Default"), DOC_MSG("Marker color name"), TRANSF) \
  XX(MessageName, float, alpha, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 1.0f), DOC_MSG("Marker fading"), TRANSF) \
  XX(MessageName, RString, fillName, NDTString, RString, NCTNone, DEFVALUE(RString, "Solid"), DOC_MSG("Marker fill name"), TRANSF) \
  XX(MessageName, float, a, NDTFloat, float, NCTNone, DEFVALUE(float, 1.0f), DOC_MSG("Width"), TRANSF) \
  XX(MessageName, float, b, NDTFloat, float, NCTNone, DEFVALUE(float, 1.0f), DOC_MSG("Height"), TRANSF) \
  XX(MessageName, float, angle, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Rotation"), TRANSF) \
  XX(MessageName, bool, drawBorder, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Draw Border"), TRANSF)
#endif

DECLARE_NET_INDICES_SERIALIZABLE(Marker, MARKER_MSG)
DEFINE_NET_INDICES_SERIALIZABLE(Marker, MARKER_MSG)

bool IsEqualMarker(NetworkMessage *marker1, NetworkMessage *marker2)
{
  NetworkMessageMarker *msg1 = (NetworkMessageMarker *)marker1;
  NetworkMessageMarker *msg2 = (NetworkMessageMarker *)marker2;

  return msg1->_name == msg2->_name;
}

bool IsEqualMarker(RString name, NetworkMessage *marker2)
{
  NetworkMessageMarker *msg = (NetworkMessageMarker *)marker2;
  return name==msg->_name;
}

NetworkMessageFormat &ArcadeMarkerInfo::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  MARKER_MSG(Marker, MSG_FORMAT)
  return format;
}

TMError ArcadeMarkerInfo::TransferMsg(NetworkMessageContext &ctx)
{
  PREPARE_TRANSFER(Marker)

  TRANSF_EX(position, position)
  TRANSF_EX(name, name)
  TRANSF_EX(text, text)
  TRANSF_ENUM_EX(markerType, markerType)
  TRANSF_EX(type, type)
  TRANSF_EX(colorName, colorName)
  TRANSF_EX(alpha, alpha)
  TRANSF_EX(fillName, fillName)
  TRANSF_EX(a, a)
  TRANSF_EX(b, b)
  TRANSF_EX(angle, angle)
  TRANSF_EX(drawBorder, drawBorder)
#if _VBS2 // conditional markers
  TRANSF_EX(autosize, autosize)
  TRANSF_EX(attachCondition, attachCondition)
  TRANSF_EX(attachOffset, attachOffset)
  TRANSF_REF_EX(attachTo, attachTo)
  TRANSF_REFS_EX(attachCtx, attachCtx)
  TRANSF_EX(condition, condition)
  TRANSF_EX(isHidden, isHidden)
  AutoArray<RString> t; 
  AutoArray<float> sA, sB, oX, oY;  
  if (ctx.IsSending())
  {
    for (int i=0; i<layeredIcons.Size(); i++)
    {
      if (layeredIcons[i] && layeredIcons[i]->icon)
      {
        t.Add("\\" + layeredIcons[i]->icon->GetName());
        sA.Add(layeredIcons[i]->a);
        sB.Add(layeredIcons[i]->b);
        oX.Add(layeredIcons[i]->x);
        oY.Add(layeredIcons[i]->y);
      }
    }
    TRANSF_EX(layeredIcons, t)
    TRANSF_EX(layeredIconsSizeA, sA)
    TRANSF_EX(layeredIconsSizeB, sB)
    TRANSF_EX(layeredIconsOffsetX, oX)
    TRANSF_EX(layeredIconsOffsetY, oY)
  }
  else
  {
    TRANSF_EX(layeredIcons, t)
    TRANSF_EX(layeredIconsSizeA, sA)
    TRANSF_EX(layeredIconsSizeB, sB)
    TRANSF_EX(layeredIconsOffsetX, oX)
    TRANSF_EX(layeredIconsOffsetY, oY)
    layeredIcons.Clear();
    for (int i=0; i<t.Size(); i++)
    {
      int j = layeredIcons.Add(new LayeredMarkerInfo());
      LayeredMarkerInfo *newLayer = layeredIcons[j];
      newLayer->icon = GlobLoadTexture (GetPictureName(t[i]));
      newLayer->a = sA[i];
      newLayer->b = sB[i]; 
      newLayer->x = oX[i]; 
      newLayer->y = oY[i]; 
    }
  }
#endif
  if (!ctx.IsSending())
  {
    OnTypeChanged();
    OnColorChanged();
    OnFillChanged();
  }
  return TMOK;
}

/*!
\patch 5088 Date 11/15/2006 by Bebul
- Fixed: Message: No entry 'bin\config.bin/CfgMarkers.'
*/

void ArcadeMarkerInfo::OnColorChanged()
{
  if (markerType == MTIcon && stricmp(colorName, "Default") == 0)
  {
    if (!type.IsEmpty())
    {
      ParamEntryVal cls = Pars >> "CfgMarkers" >> type;
      color = GetPackedColor(cls >> "color");
    }
  }
  else
  {
    ParamEntryVal cls = Pars >> "CfgMarkerColors" >> colorName;
    color = GetPackedColor(cls >> "color");
  }
}

void ArcadeMarkerInfo::OnFillChanged()
{
  ParamEntryVal cls = Pars >> "CfgMarkerBrushes" >> fillName;
  RString brush = cls >> "texture";
  drawBorder =  cls >> "drawBorder";

  if (brush.GetLength() == 0)
    fill = NULL;
  else
    fill = GlobLoadTexture(GetPictureName(brush));
}

void ArcadeMarkerInfo::OnTypeChanged()
{
  if (markerType == MTIcon)
  {
    if (!type.IsEmpty())
    {
      ParamEntryVal cls = Pars >> "CfgMarkers" >> type;
      icon = GlobLoadTexture
        (
        GetPictureName(cls >> "icon")
        );
      size = cls >> "size";
      if(cls.FindEntry("shadow")) shadow = cls >> "shadow";
      else shadow = true;

      if (stricmp(colorName, "Default") == 0)
      {
        color = GetPackedColor(cls >> "color");
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// struct ArcadeEffects

ArcadeEffects::ArcadeEffects()
{
  Init();
}

ArcadeEffects::ArcadeEffects(const ArcadeEffects &src)
{
  condition = src.condition;
  sound = src.sound;
  voice = src.voice;
  soundEnv = src.soundEnv;
  soundDet = src.soundDet;
  track = src.track;
  titleType = src.titleType;
  titleEffect = src.titleEffect;
  title = src.title;
}

void ArcadeEffects::Init()
{
  condition = "true";
  sound = "$NONE$";
  voice = "";
  soundEnv = "";
  soundDet = "";
  track = "$NONE$";
  titleType = TitleNone;
  titleEffect = TitPlain;
  title = "";
}

LSError ArcadeEffects::Serialize(ParamArchive &ar)
{
  if (ar.IsSaving() || ar.GetArVersion() >= 9)
    CHECK(ar.Serialize("condition", condition, 9, "true"))
  else
  {
    bool playerOnly;
    CHECK(ar.Serialize("playerOnly", playerOnly, 1, false)) 
    if (playerOnly)
      condition = "thisList";
    else
      condition = "true";
  }

  CHECK(ar.Serialize("sound", sound, 1, "$NONE$"))
  CHECK(ar.Serialize("voice", voice, 5, ""))
  CHECK(ar.Serialize("soundEnv", soundEnv, 5, ""))
  CHECK(ar.Serialize("soundDet", soundDet, 5, ""))
  CHECK(ar.Serialize("track", track, 1, "$NONE$"))

  CHECK(ar.SerializeEnum("titleType", titleType, 1, TitleNone)) 
  CHECK(ar.SerializeEnum("titleEffect", titleEffect, 1, ENUM_CAST(TitEffectName,TitPlain)))
  CHECK(ar.Serialize("title", title, 1, ""))  

  return LSOK;
}

LSError ArcadeEffects::WorldSerialize(ParamArchive &ar)
{
  if (ar.IsSaving() || ar.GetArVersion() >= 5)
    CHECK(ar.Serialize("condition", condition, 5, "true"))
  else
  {
    bool playerOnly;
    CHECK(ar.Serialize("playerOnly", playerOnly, 1, false)) 
    if (playerOnly)
      condition = "thisList";
    else
      condition = "true";
  }

  CHECK(ar.Serialize("sound", sound, 1, "$NONE$"))
  CHECK(ar.Serialize("voice", voice, 1, ""))
  CHECK(ar.Serialize("soundEnv", soundEnv, 1, ""))
  CHECK(ar.Serialize("soundDet", soundDet, 1, ""))
  CHECK(ar.Serialize("track", track, 1, "$NONE$"))

  CHECK(ar.SerializeEnum("titleType", titleType, 1, TitleNone)) 
  CHECK(ar.SerializeEnum("titleEffect", titleEffect, 1, ENUM_CAST(TitEffectName,TitPlain)))
  CHECK(ar.Serialize("title", title, 1, ""))  

  return LSOK;
}

DEFINE_NETWORK_OBJECT_SIMPLE(ArcadeEffects, Effects)

#define EFFECTS_MSG(MessageName, XX) \
  XX(MessageName, RString, condition, NDTString, RString, NCTNone, DEFVALUE(RString, "true"), DOC_MSG("Condition when effect is performed"), TRANSF) \
  XX(MessageName, RString, sound, NDTString, RString, NCTNone, DEFVALUE(RString, "$NONE$"), DOC_MSG("Sound effect (2D)"), TRANSF) \
  XX(MessageName, RString, voice, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Sound effect (3D)"), TRANSF) \
  XX(MessageName, RString, soundEnv, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Enviromental sound effect"), TRANSF) \
  XX(MessageName, RString, soundDet, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Detector sound effect"), TRANSF) \
  XX(MessageName, RString, track, NDTString, RString, NCTNone, DEFVALUE(RString, "$NONE$"), DOC_MSG("Musical track"), TRANSF) \
  XX(MessageName, int, titleType, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, TitleNone), DOC_MSG("Type of title effect (text, object, resource)"), TRANSF) \
  XX(MessageName, int, titleEffect, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, TitPlain), DOC_MSG("Type (placement) of text title effect"), TRANSF) \
  XX(MessageName, RString, title, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Content of title effect"), TRANSF)

DECLARE_NET_INDICES_INIT_MSG(Effects, EFFECTS_MSG)
DEFINE_NET_INDICES_INIT_MSG(Effects, EFFECTS_MSG)
//DECLARE_NET_INDICES(Effects, EFFECTS_MSG)
//DEFINE_NET_INDICES(Effects, EFFECTS_MSG)

NetworkMessageFormat &ArcadeEffects::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  EFFECTS_MSG(Effects, MSG_FORMAT)
  return format;
}

TMError ArcadeEffects::TransferMsg(NetworkMessageContext &ctx)
{
  PREPARE_TRANSFER(Effects)

  TRANSF_EX(condition, condition)
  TRANSF_EX(sound, sound)
  TRANSF_EX(voice, voice)
  TRANSF_EX(soundEnv, soundEnv)
  TRANSF_EX(soundDet, soundDet)
  TRANSF_EX(track, track)
  TRANSF_ENUM_EX(titleType, titleType)
  TRANSF_ENUM_EX(titleEffect, titleEffect)
  TRANSF_EX(title, title)
  return TMOK;
}

///////////////////////////////////////////////////////////////////////////////
// struct ArcadeIntel

ArcadeIntel::ArcadeIntel()
{
  Init();
}

void ArcadeIntel::Init()
{
  friends[TEast][TEast] = 1.0;      friends[TEast][TWest] = 0.0;      friends[TEast][TGuerrila] = 0.0;
  friends[TWest][TEast] = 0.0;      friends[TWest][TWest] = 1.0;      friends[TWest][TGuerrila] = 1.0;
  friends[TGuerrila][TEast] = 0.0;  friends[TGuerrila][TWest] = 1.0;  friends[TGuerrila][TGuerrila] = 1.0;
  
  ConstParamEntryPtr cfg = NULL;
  if (*Glob.header.worldname) cfg = (Pars >> "CfgWorlds").FindEntry(Glob.header.worldname);

  year = 1985;
  month = 5;
  day = 10;
  hour = 7;
  minute = 30;

  if (cfg)
  {
    // initialize from the world config
    weather = *cfg >> "startWeather";
    weatherForecast = *cfg >> "forecastWeather";
    fog = *cfg >> "startFog";
    fogForecast = *cfg >> "forecastFog";
    wind = *cfg >> "startWind";
    windForecast = *cfg >> "forecastWind";

    RString date = *cfg >> "startDate";
    RString time = *cfg >> "startTime";
    sscanf(date, "%d/%d/%d", &day, &month, &year);
    if (year < 100) year += 1900;
    sscanf(time, "%d:%d", &hour, &minute);
  }
  else
  {
    weather = 0.5;
    fog = 0;
    wind = 0.1;
    weatherForecast = 0.5;
    fogForecast = 0;
    windForecast = 0.1;    

  }
  viewDistance = 0;

  briefingName = "";
  briefingDescription = "";
}

void SendIntel(ArcadeIntel *intel);

LSError ArcadeIntel::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("briefingName", briefingName, 2, ""))  
  CHECK(ar.Serialize("briefingDescription", briefingDescription, 2, ""))  

  if (ar.GetArVersion() >= 10)
  {
    CHECK(ar.Serialize("resistanceWest", friends[TWest][TGuerrila], 1, 1.0))
    CHECK(ar.Serialize("resistanceEast", friends[TEast][TGuerrila], 1, 0.0))
  }
  else
  {
    CHECK(ar.Serialize("resistance", friends[TWest][TGuerrila], 1, 1.0))
    friends[TEast][TGuerrila] = 1.0f - friends[TWest][TGuerrila];
  }
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    friends[TEast][TEast] = 1.0f; friends[TEast][TWest] = 0.0f;
    friends[TWest][TEast] = 0.0f; friends[TWest][TWest] = 1.0f;
    friends[TGuerrila][TGuerrila] = 1.0f;

    friends[TGuerrila][TEast] = friends[TEast][TGuerrila];
    friends[TGuerrila][TWest] = friends[TWest][TGuerrila];
  }

  CHECK(ar.Serialize("startWeather", weather, 1, 0.5))  
  CHECK(ar.Serialize("startFog", fog, 1, 0))  
  CHECK(ar.Serialize("startWind", wind, 1, 0.1))  
  CHECK(ar.Serialize("forecastWeather", weatherForecast, 1, 0.5)) 
  CHECK(ar.Serialize("forecastFog", fogForecast, 1, 0.0)) 
  CHECK(ar.Serialize("forecastWind", windForecast, 1, 0.1)) 
  CHECK(ar.Serialize("viewDistance", viewDistance, 1, 0)) 
  CHECK(ar.Serialize("year", year, 1, 1985))  
  CHECK(ar.Serialize("month", month, 1, 5)) 
  CHECK(ar.Serialize("day", day, 1, 10))  
  CHECK(ar.Serialize("hour", hour, 1, 7)) 
  CHECK(ar.Serialize("minute", minute, 1, 30)) 

  return LSOK;
}

///////////////////////////////////////////////////////////////////////////////
// class ArcadeTemplate

ArcadeTemplate::ArcadeTemplate()
{
  showHUD = true;
  showMap = true;
  showWatch = true;
  showCompass = true;
  showNotepad = true;
  showGPS = false;

  nextVehId = 0;
}

bool ArcadeTemplate::IsEmpty() const
{
  // FindPlayer will find ambient ("independent agents") as well
  const ArcadeUnitInfo *uInfo = FindPlayer(); 
  if (uInfo) return false;
  return emptyVehicles.Size() == 0;
}


/*!
  \patch_internal 1.01 Date 06/19/2001 by Jirka - added check of required AddOns
*/
void ArcadeTemplate::RequiredAddons(FindArrayRStringCI &addOns, RString worldName)
{
  for (int i=0; i<emptyVehicles.Size(); i++) emptyVehicles[i].RequiredAddons(addOns);
  // island/world - check owner of the island config
  ConstParamEntryPtr entry = (Pars>>"CfgWorlds").FindEntry(worldName);
  if (entry)
  {
    const RStringB &owner = entry->GetOwnerName();
    if (owner.GetLength()>0)
    {
      addOns.AddUnique(owner);
    }
  }
}

void CheckPatch(FindArrayRStringCI &addOns, FindArrayRStringCI &missing)
{
  ConstParamEntryPtr patches = Pars.FindEntry("CfgPatches");

  // activate all addons requested by given mission
  GWorld->ActivateAddons(addOns,RString());

  if (patches)
  {
    int m = patches->GetEntryCount();

    for (int i=0; i<addOns.Size(); i++)
    {
      RString addOn = addOns[i];
      bool found = false;
      for (int j=0; j<m; j++)
      {
        if (stricmp(addOn, patches->GetEntry(j).GetName()) == 0)
        {
          found = true; break;
        }
      }
      if (!found) missing.Add(addOn);
    }
  }

  if (missing.Size() > 0)
  {
    RptF("Missing addons detected:");
    for (int i=0; i<missing.Size(); i++)
    {
      RptF("  %s", (const char *)missing[i]);
    }
  }
}

/*!
\patch 5153 Date 4/17/2007 by Jirka
- Fixed: UI - Missing addons error message - show the list of missing addons
*/

/// display message about missing addons
RString ReportMissingAddons(FindArrayRStringCI missing)
{
  RString message = LocalizeString(IDS_MSG_ADDON_MISSING);
  return message;
}

/// check if some addons are missing, display message if so
bool CheckMissingAddons(FindArrayRStringCI &addOns)
{
  FindArrayRStringCI missing;
  CheckPatch(addOns, missing);
  if (missing.Size() > 0)
  {
    RString message = ReportMissingAddons(missing);
    WarningMessage(message);
    return false;
  }
  return true;
}

/*!
\patch 1.82 Date 8/15/2002 by Ondra
- Fixed: Addon was not removed from addons[] list
when last unit using that addon was removed in the mission editor.
*/

void ArcadeTemplate::ScanRequiredAddons(RString worldName)
{
  FindArrayRStringCI oldAddOnsAuto = addOnsAuto;
  addOnsAuto.Resize(0);
  RequiredAddons(addOnsAuto,worldName);
  // check which items in oldAddOnsAuto are no longer listed in addOnsAuto
  // such items can be removed from addon list
  for (int i=0; i<oldAddOnsAuto.Size(); i++)
  {
    RString old = oldAddOnsAuto[i];
    if (addOnsAuto.Find(old)<0)
    {
      // was listed in old list, but is not in new
      // we can remove it from the required list as 
      // it might be listed several times?
      addOns.Delete(old);
    }
  }
  for (int i=0; i<addOnsAuto.Size(); i++)
  {
    addOns.AddUnique(addOnsAuto[i]);
  }
}

/*!
\patch 1.30 Date 11/03/2001 by Jirka
- Fixed: Check of AddOns in multiplayer game
\patch 1.63 Date 6/2/2002 by Ondra
- Improved: Addons added by mission editor as required are also removed
by the mission editor.
*/


LSError ArcadeTemplate::Serialize(ParamArchive &ar)
{
  ATSParams *params = (ATSParams *)ar.GetParams();
  if (ar.IsSaving())
  {
    // ADDED - AddOns check
    // hack to enable scripted missions to work in MP
    if (!params->avoidScanAddons)
      ScanRequiredAddons(Glob.header.worldname);
    // addOns.Resize(0);
    // items from this list can be removed if necessary
    CHECK(ar.SerializeArray("addOns", addOns, 1))
    CHECK(ar.SerializeArray("addOnsAuto", addOnsAuto, 1))

    if (!params->avoidCheckSync)
    {
      Compact();
    }
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    // ADDED - AddOns check
    addOns.Resize(0);
    missingAddOns.Resize(0);
    CHECK(ar.SerializeArray("addOns", addOns, 1))
    CheckPatch(addOns, missingAddOns);
    if (missingAddOns.Size() > 0)
    {
      return LSNoAddOn;
    }

    if (!params)
    {
      Fail("Params needed");
      return LSStructure;
    }
    FindArrayRStringCI addOnsBackup = addOns;
    Clear();
    addOns = addOnsBackup;
    params->nextVehId = 0;
  }

  CHECK(ar.Serialize("showHUD", showHUD, 8, true))
  CHECK(ar.Serialize("showMap", showMap, 8, true))
  CHECK(ar.Serialize("showWatch", showWatch, 8, true))
  CHECK(ar.Serialize("showCompass", showCompass, 8, true))
  CHECK(ar.Serialize("showNotepad", showNotepad, 8, true))
  CHECK(ar.Serialize("showGPS", showGPS, 8, false))
  CHECK(ar.Serialize("randomSeed", randomSeed, 1, 1))

  CHECK(ar.Serialize("Intel", intel, 7))
  CHECK(ar.Serialize("Vehicles", emptyVehicles, 1))
  CHECK(ar.Serialize("Markers", markers, 1))

  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    Assert(params);
    nextVehId = params->nextVehId;
    if (!params->avoidCheckSync)
    {
      Compact();
    }
  }
  return LSOK;
}

const ArcadeUnitInfo *ArcadeTemplate::FindPlayer() const
{
  for (int i=0; i<emptyVehicles.Size(); i++)
  {
    const ArcadeUnitInfo &uInfo = emptyVehicles[i];
    switch (uInfo.player)
    {
    case APPlayerCommander:
    case APPlayerDriver:
    case APPlayerGunner:
      return &uInfo;
    }
  }
  return NULL;
}

void ArcadeTemplate::Clear()
{
  emptyVehicles.Clear();
  markers.Clear();
  
  showHUD = true;
  showMap = true;
  showWatch = true;
  showCompass = true;
  showNotepad = true;
  showGPS = false;

  nextVehId = 0;

  addOns.Clear();
  missingAddOns.Clear();

  // assign a new random seed
  randomSeed = toLargeInt(GRandGen.RandomValue()*0x1000000)+3;
}

struct ConversionItem
{
  bool used;
  int newIndex;
};
TypeIsSimple(ConversionItem)

/// compact vehicle and synchronization IDs to use small numbers
void ArcadeTemplate::Compact()
{
  AutoArray<ConversionItem> table;
  
  // vehicle IDs

  // find what IDs are used
  table.Resize(nextVehId);
  for (int i=0; i<nextVehId; i++)
  {
    table[i].used = false;
    table[i].newIndex = -1;
  }
  for (int i=0; i<emptyVehicles.Size(); i++)
  {
    ArcadeUnitInfo &uInfo = emptyVehicles[i];
    Assert(uInfo.id >= 0);
    Assert(uInfo.id < nextVehId);
    table[uInfo.id].used = true;
  }

  // create a mapping table
  int index = 0;
  for (int i=0; i<nextVehId; i++)
    if (table[i].used) table[i].newIndex = index++;
  nextVehId = index;
}

