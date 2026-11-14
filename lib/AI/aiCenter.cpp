// AI - implementation of AI and AICenter

#include "../wpch.hpp"

#include <El/Enum/enumNames.hpp>
#include "ai.hpp"
#include "../Network/network.hpp"
#include "../arcadeTemplate.hpp"
#include "../detector.hpp"
#include "../dynSound.hpp"
#include "../engine.hpp"
#include "../gameStateExt.hpp"
#include "../mbcs.hpp"
#include "../shots.hpp"
#include "../stringtableExt.hpp"
#include "../world.hpp"

#include "../Inventory/magazine.hpp"
#include "../UI/missionDirs.hpp"
#include "El/ParamArchive/paramArchiveDb.hpp"

///////////////////////////////////////////////////////////////////////////////
// Parameters

extern const float AccuracyLast = 7200;             // how long we can remember the information
extern const float ExposureTimeout = 450;  // how long exposure persist when target is not visible

#define LOG_TARGETS 0

extern const float CoefAdvanceUnit = 200000;

static const EnumName EndModeNames[]=
{
  EnumName(EMContinue,"CONTINUE"),
  EnumName(EMKilled,"KILLED"),
  EnumName(EMLoser,"LOSER"),
  EnumName(EMCheat, "ENDDEFAULT"),
  EnumName(EMEnd1,"END1"),
  EnumName(EMEnd2,"END2"),
  EnumName(EMEnd3,"END3"),
  EnumName(EMEnd4,"END4"),
  EnumName(EMEnd5,"END5"),
  EnumName(EMEnd6,"END6"),
  EnumName()
};

#ifdef _LINUX
template<> const EnumName *GetEnumNames(EEndMode_Enum dummy)
{
  return EndModeNames;
}
#endif
template<> const EnumName *GetEnumNames(EEndMode dummy)
{
  return EndModeNames;
}

SRef<IMission> CurrentTemplate = new DummyMission();

int GetTemplateSeed()
{
  return CurrentTemplate->RandomSeed();
}

RString CurrentMission;
RString CurrentWorld;
RString CurrentFile;

AutoArray<ArcadeMarkerInfo> markersMap;

void AIGlobalCleanUp()
{
  markersMap.Clear();
  void ResetMarkerID();
  ResetMarkerID();
}

// Init / load / save
void AIGlobalInit()
{
  AIGlobalCleanUp();
}

LSError AIGlobalSerialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("Stats", GStats, 1))
  CHECK(ar.Serialize("Markers", markersMap, 1))
  return LSOK;
}

#define MIN_DIST2   50.0F * 50.0F
bool ValidPos(Vector3Val pos, AutoArray<Point3> &forbidden)
{
  int i;
  for (i=0; i<forbidden.Size(); i++)
    if ((forbidden[i] - pos).SquareSizeXZ() < MIN_DIST2)
      return false;
  return true;
}

struct StartingPoint
{
  Vector3 _pos;
  float _cost;

  StartingPoint(){}
  StartingPoint(Vector3Par pos, float cost) : _pos(pos), _cost(cost) {}
};
TypeIsSimple(StartingPoint)

int CmpStartingPoints(const StartingPoint *pt1, const StartingPoint *pt2)
{
  float value = pt1->_cost - pt2->_cost;
  return toInt(fSign(value));
}

static Vector3 FindStartPosition(Vector3Val center, float radius, AutoArray<Point3> *avoid = NULL)
{
  if (radius <= 0.1) return center;

  //AutoArray<Point3> points;
  // use paper car - it is cheap in terms of textures and memory
  // it is defined so that it gives most constraints on position of all all vehicles
/*
  Vector3 bestPoint = center;
  bool bestPointValid = false;
  int j;
  float minCost=1e10;
*/
  AUTO_STATIC_ARRAY_16(StartingPoint, points, 100);
  for (int j=0; j<100; j++)
  {
    float xr = (2.0 * GRandGen.RandomValue() - 1.0) * radius;
    float zr = (2.0 * GRandGen.RandomValue() - 1.0) * radius;
    if (Square(xr) + Square(zr) > radius * radius) continue;
    Point3 pos;   
    pos.Init();
    pos[0] = center[0] + xr;
    pos[1] = 0;
    pos[2] = center[2] + zr;
    int xtest = toIntFloor(pos.X() * InvLandGrid);
    int ztest = toIntFloor(pos.Z() * InvLandGrid);
    for (int x=-1; x<=1; x++)
      for (int z=-1; z<=1; z++)
      {
        GeographyInfo info = GLOB_LAND->GetGeography(xtest + x, ztest + z);
        if (GWorld->Preloaded(VTypePaperCar)->GetBaseCost(info, false, true) > 1e10)
          goto Break;
      }
    if (avoid && !ValidPos(pos, *avoid))
        continue;

    {
      GeographyInfo info = GLOB_LAND->GetGeography(xtest, ztest);
      float cost = GWorld->Preloaded(VTypePaperCar)->GetBaseCost(info, false, true);
      /*
      if (cost < minCost)
      {
        minCost = cost;
        bestPoint = pos;
        bestPointValid = true;
      }
      */
      points.Add(StartingPoint(pos, cost));
    }

Break:
    continue;
  }
  if (points.Size() == 0)
  {
    //Fail("No valid start position.");
    return center;
  }

  QSort(points.Data(), points.Size(), CmpStartingPoints);
  float limit = 5.0f * points[0]._cost;
  int n = points.Size();
  for (int i=1; i<points.Size(); i++)
  {
    if (points[i]._cost > limit)
    {
      n = i;
      break;
    }
  }

  int index = toIntFloor(n * GRandGen.RandomValue());
  saturate(index, 0, n - 1);
  return points[index]._pos;
}

struct MagazineInfo
{
  const MagazineType *type;
  RefArray<Magazine> list;
};
TypeIsMovable(MagazineInfo)

class SetAmmoFunc : public ITurretFunc
{
protected:
  float _amount;

public:
  SetAmmoFunc(float amount) : _amount(amount) {}
  bool operator () (EntityAIFull *entity, TurretContext &context)
  {
    AUTO_STATIC_ARRAY(MagazineInfo, infos, 32);
    // loaded magazines
    context._weapons->ForEachMuzzle([&infos](const WeaponModePtr &p) -> bool
    {
      const Magazine *magazine = p.GetMagazine();
      if (!magazine) return false; // continue
      const MagazineType *type = magazine->Type();
      if (!type || type->_maxAmmo == 0) return false; // continue
      bool found = false;
      for (int j=0; j<infos.Size(); j++)
      {
        if (infos[j].type == type)
        {
          found = true;
          infos[j].list.Add(unconst_cast(magazine));
          break;
        }
      }
      if (!found)
      {
        int index = infos.Add();
        infos[index].type = type;
        infos[index].list.Add(unconst_cast(magazine));
      }
      return false; // continue
    });
    // other magazines
    context._weapons->ForEachMagazine([&infos](Magazine *magazine) -> bool
    {
      const MagazineType *type = magazine->Type();
      if (!type || type->_maxAmmo == 0) return false; // continue
      bool found = false;
      for (int j=0; j<infos.Size(); j++)
      {
        if (infos[j].type == type)
        {
          found = true;
          infos[j].list.Add(magazine);
          break;
        }
      }
      if (!found)
      {
        int index = infos.Add();
        infos[index].type = type;
        infos[index].list.Add(magazine);
      }
      return false; // continue
    });
    for (int k=0; k<infos.Size(); k++)
    {
      const MagazineType *type = infos[k].type;
      if (!type) continue;
      int ammo = toInt(_amount * infos[k].list.Size() * type->_maxAmmo);
      for (int i=0; i<infos[k].list.Size(); i++)
      {
        Magazine *magazine = infos[k].list[i];
        if (ammo == 0)
        {
          context._weapons->RemoveMagazine(entity, magazine);
        }
        else if (ammo >= type->_maxAmmo)
        {
          magazine->SetAmmo(type->_maxAmmo);
          ammo -= type->_maxAmmo;
        }
        else
        {
          magazine->SetAmmo(ammo);
          ammo = 0;
        }
      }
    }

    return false; // all turrets
  }
};

void SetAmmo(EntityAIFull *veh, float amount)
{
  Assert(veh);
  SetAmmoFunc func(amount);
  veh->ForEachTurret(func);
}

static Matrix4 PlaceVehicle(EntityAI *veh, Vector3 pos, Matrix3Par rotation, bool flying, bool playerIn)
{
  const EntityAIType *type = veh->GetType();

  // align position to the surface, calculate the normal as well
  Vector3 normal;
  if (
    // enable placing soldier onto object road
    type->IsPerson()
    // enable placing aircraft or helipad onto carrier deck (roadway above water)
    || (type->CanBeAirborne() || type->IsKindOf(GWorld->Preloaded(VTypeHelipad))) && GLandscape->SurfaceY(pos)<GLandscape->GetSeaLevel()-2
  )
  {
    pos[1] = GLOB_LAND->RoadSurfaceYAboveWater(pos[0], pos[2]);
    normal = VUp;
    if (pos[1]>GLandscape->SurfaceY(pos))
    {
      veh->SetNotOnSurface(true);
  }
  }
  else
  {
    float dx, dz;
    pos[1] = GLOB_LAND->SurfaceYAboveWater(pos[0], pos[2], &dx, &dz);
    normal = Vector3(-dx, 1, -dz);
  }

  Matrix3 dir;
  Matrix4 transform;

  if (
    // Fix: enable empty flying parachutes
    // side != TSideUnknown &&
    flying && type->IsKindOf(GWorld->Preloaded(VTypeAir))
    )
  {
    // orientation
    normal = VUp;
    dir.SetUpAndDirection(normal, VForward);
    transform.SetOrientation(dir * rotation);

    // position
    pos[1] += veh->MakeAirborne(transform.Orientation().Direction(), playerIn);
    transform.SetPosition(pos);
  }
  else
  {
    // orientation
    dir.SetUpAndDirection(normal, VForward);
    transform.SetOrientation(dir * rotation);

    // position
    transform.SetPosition(pos);

    // update the transformation
    veh->PlaceOnSurface(transform, !veh->CanFloat());
  }

  return transform;
}

extern const float MinHealth=0.03;

static EntityAI *CreateVehicle(const ArcadeUnitInfo &info, int randomSeed, TargetSide side, bool multiplayer, bool disableD)
{
  FreeOnDemandGarbageCollect(2*1024*1024,256*1024);
  
  // avoid sounds
  RString vehClass;
  if( info.type )
  {
    vehClass=info.type->_simName;
  }
  else
  {
    ConstParamEntryPtr entry = (Pars >> "CfgVehicles").FindEntry(info.vehicle);
    if (entry) vehClass =  (*entry)>>"vehicleClass";
    else
    {
      RptF("Vehicle type %s does not exist",cc_cast(info.vehicle));
      return NULL;
    }
  }
  if (stricmp(vehClass, "Sounds") == 0) return NULL;

  Ref<EntityType> type(info.type);
  if (!type) type = VehicleTypes.New(info.vehicle);
  if (!type) return NULL;
  ProgressRefresh();

  bool isSoldier = type->IsPerson();
  if (disableD && isSoldier) return NULL;

  Ref<EntityAI> veh;
  if( info.type )
  {
    veh = GWorld->NewVehicleWithID(info.type);
  }
  else
  {
    veh = GWorld->NewVehicleWithID(info.vehicle);
  }
  if (!veh) return NULL;
  ProgressRefresh();

  Vector3 pos = info.position;

  // find some position inside a given radius
  pos = FindStartPosition(pos, info.placement);

  float azimut = -HDegree(info.azimut);
  ProgressRefresh();

  if (side != TSideUnknown) veh->SetTargetSide(side);

  if (info.name.GetLength() > 0)
  {
    veh->SetVarName(info.name);
    GWorld->GetGameState()->VarSet(info.name, GameValueExt(veh.GetRef()), true, false, GWorld->GetMissionNamespace()); // mission namespace
  }
  ProgressRefresh();
  
  // Add into World
//  veh->Repair(floatMax(MinHealth, info.health) - 1 - veh->GetTotalDamage());
  veh->SetDamage(1 - floatMax(MinHealth, info.health), true);
  veh->Refuel(info.fuel * veh->GetType()->GetFuelCapacity() - veh->GetFuel());
  ProgressRefresh();

  EntityAIFull *vehFull = dyn_cast<EntityAIFull>(veh.GetRef());
  if (vehFull)
  {
    SetAmmo(vehFull, info.ammo);
    vehFull->SetDescription(info.description);
  }
  ProgressRefresh();

  Matrix3 rotY(MRotationY, azimut);

  Vector3 normalDummy;
  if (info.placement > 0)
  {
    if (!AIBrain::FindFreePosition(pos, normalDummy, isSoldier, veh->GetType()))
    {
      Fail("Collision in the starting position");
    }
  }

  //info if player is on vehicle
  bool playerIn = (info.player == APPlayerCommander) || (info.player == APPlayerDriver) || (info.player == APPlayerGunner);
  // calculate the transformation
  Matrix4 transform = PlaceVehicle(veh, pos, rotY, info.special == ASpFlying, playerIn);
  ProgressRefresh();

  // apply the transformation
  veh->SetTransformPossiblySkewed(transform);
  veh->Init(transform, true);

  ProgressRefresh();

  // if vehicle is static, add it to building list

  if (veh->GetType()->IsSlow())
  {
    GWorld->AddSlowVehicle(veh);
    if (multiplayer)
      GetNetworkManager().CreateVehicle(veh, VLTSlowVehicle, info.name, info.id);
  }
  else
  {
    GWorld->AddVehicle(veh);     
    if (multiplayer)
      GetNetworkManager().CreateVehicle(veh, VLTVehicle, info.name, info.id);

  }

  // after CreateVehicle: the event handler may reference the object being created, it needs to exists as a network object
  veh->OnEvent(EEInit);

  Transport *transport = dyn_cast<Transport>(veh.GetRef());
  if (transport)
  {
//    transport->SetLocked(info.locked);
    transport->SetLock(info.lock);

    // generate plate number from "regulations"
    ParamEntryVal world=Pars>>"CfgWorlds">> Glob.header.worldname;
    RString format = world>>"plateFormat";
    RString letters = world>>"plateLetters";

    int seed = info.id*4586 + randomSeed*871;

    int plateId = toLargeInt(GRandGen.RandomValue(seed)*0x1000000);

    int nLetters = letters.GetLength();

    char plate[256];
    char *p=plate;
    for (const char *f=format; *f; f++)
    {
      if(*f=='$')
      {
        *p++ = letters[plateId%nLetters];
        plateId/=nLetters;
      }
      else if (*f=='#')
      {
        *p++ = '0'+plateId%10;
        plateId/=10;
      }
      else
      {
        *p++=*f;
      }
    }
    *p=0;
    // offer the plate to the vehicle
    transport->SetPlateNumber(plate);
  }

  return veh;
}

void CreateMarker(const ArcadeMarkerInfo &info, bool multiplayer)
{
  markersMap.Add(info);
}

static void InitPlayer(AIBrain *playerUnit)
{
  playerUnit->GetPerson()->SetRemotePlayer(AI_PLAYER);

  // avoid identity overwrite when delay EH processed
  playerUnit->LoadIdentityDelayed();

  AIUnitInfo &info = playerUnit->GetPerson()->GetInfo();
#if _VBS2 // VBS2 nickname
  info._fullName = Glob.header.playerHandle;
#else
  info._fullName = Glob.header.GetPlayerName();
#endif
  info._face = Glob.header.playerFace;
  info._glasses = Glob.header.playerGlasses;
  info._speaker = playerUnit->GetPerson()->SelectSpeaker(Glob.header.playerSpeakerType);
  info._pitch = Glob.header.playerPitch;

  playerUnit->GetPerson()->SetFace(info._face);
  playerUnit->GetPerson()->SetGlasses(info._glasses);
  playerUnit->SetSpeaker(info._speaker, info._pitch);
};

void InitNoCenters(const ArcadeTemplate &t, VehicleInitMessages &inits, GameMode mode)
{
  AIBrain *playerUnit = NULL;

  // empty vehicles
  int m = t.emptyVehicles.Size();
  for (int j=0; j<m; j++)
  {
    const ArcadeUnitInfo &uInfo = t.emptyVehicles[j];
    if (GRandGen.RandomValue() > uInfo.presence) continue;
    if (!GWorld->GetGameState()->EvaluateBool(uInfo.presenceCondition, GameState::EvalContext::_default, GWorld->GetMissionNamespace())) continue; // mission namespace

    ConstParamEntryPtr entry = (Pars >> "CfgVehicles").FindEntry(uInfo.vehicle);
    if (!entry)
    {
      RptF("Vehicle type %s does not exist",cc_cast(uInfo.vehicle));
      return;
    }
    RString vehClass =  (*entry)>>"vehicleClass";

    {
      EntityAI *veh = CreateVehicle(uInfo, t.randomSeed, TSideUnknown, mode == GModeNetware, false);
      if (veh)
      {
        // init expression
        if (uInfo.init.GetLength() > 0)
        {
          int index = inits.Add();
          inits[index]._vehicle = veh;
          inits[index]._init = uInfo.init;
        }
      }
      switch (uInfo.player)
      {
      case APPlayerCommander:
        playerUnit = veh->CommanderUnit();
        if (playerUnit)
        {
          InitPlayer(playerUnit);
        }
        break;
      case APPlayerDriver:
        playerUnit = veh->PilotUnit();
        if (playerUnit)
        {
          InitPlayer(playerUnit);
        }
        break;
      case APPlayerGunner:
        playerUnit = veh->GunnerUnit();
        if (playerUnit)
        {
          InitPlayer(playerUnit);
        }
        break;
      case APPlayableD:
      case APPlayableCD:
      case APPlayableDG:
      case APPlayableCDG:
        // simplified - CreateVehicle cannot create more units
        break;
      }
    }
    ProgressAdvance(CoefAdvanceUnit);
    ProgressRefresh();
  }
  // markers
  m = t.markers.Size();
  for (int j=0; j<m; j++)
  {
    const ArcadeMarkerInfo &mInfo = t.markers[j];
    CreateMarker(mInfo, mode == GModeNetware);
  }

  if (playerUnit)
  {
    if (mode == GModeIntro)
    {
      if (playerUnit)
        GWorld->SwitchCameraTo(playerUnit->GetVehicle(), CamExternal, true);
    }
    else
    {
      GStats._playerInfo.unit = playerUnit;

      // keep rank and exp. from editor
      if (mode != GModeNetware)
      {
        PlayerInfo().rank = playerUnit->GetPerson()->GetInfo()._rank;
        PlayerInfo().experience = playerUnit->GetPerson()->GetExperience();
      }

      #if _ENABLE_REPORT
      DWORD start = GlobalTickCount();
      #endif
      GWorld->SwitchCameraTo(playerUnit->GetVehicle(), GInput.perspective==3 ? CamExternal : CamInternal, true);
      #if _ENABLE_REPORT
      DWORD time = GlobalTickCount() - start;
      LogF("SwitchCamera takes %d ms", time);
      #endif
      GWorld->SetPlayerManual(true);
      GWorld->SwitchPlayerTo(playerUnit->GetPerson());
      GWorld->SetRealPlayer(playerUnit->GetPerson());
    }
  }
}

void InitNoCenters(IMission &t, VehicleInitMessages &inits, GameMode mode)
{
  t.InitNoCenters(inits,mode);
}

void ArcadeTemplate::InitNoCenters(VehicleInitMessages &inits, GameMode mode) const
{
  ::InitNoCenters(*this,inits,mode);
}

void ArcadeTemplate::ActivateAddons(RString worldname)
{
  GWorld->ActivateAddons(addOns,worldname);
}

void ArcadeTemplate::InitGeneral(GameMode mode)
{
  GWorld->SetMode(mode);
  GWorld->InitGeneral(intel);
}

void ArcadeTemplate::AddMarkers()
{
  int n = markers.Size();
  for (int i=0; i<n; i++) markersMap.Add(markers[i]);
}


bool ArcadeTemplate::Load(RString name, RString cutscene, GameDataNamespace &globals, GameVarSpace &local)
{
  ParamArchiveLoad ar;
  if (!ar.LoadBin(name, &local, &globals) && !ar.Load(name, &local, &globals))
  {
    return false;
  }


  ATSParams params;
  params.avoidCheckSync = true;
  ar.SetParams(&params);
  LSError result = ar.Serialize(cutscene, *this, 1);
  if (result==LSOK && !GetMissionParametersDisplayName().IsEmpty())
  {
    intel.briefingName = GetMissionParametersDisplayName(); //override
  }
  if (result == LSNoAddOn)
  {
    RString ReportMissingAddons(FindArrayRStringCI missing);
    RString message = ReportMissingAddons(missingAddOns);
    WarningMessage(message);
  }
  else if (result != LSOK)
  {
    WarningMessage("Cannot load mission");
  }
  if (ar.GetArVersion() < 7)
  {
    if (ar.Serialize("Intel", intel, 1) != LSOK)
    {
      WarningMessage("Cannot load intel (old mission format)");
    }
  }
  return true;
}



MissionWithParams::MissionWithParams()
{
  randomSeed = toLargeInt(GRandGen.RandomValue()*0x1000000)+3;
}


bool MissionWithParams::ParseMissionParams(RString missionParamsName, GameVarSpace &local)
{
  GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

  // mission parameters file
  ParamFile missionParams;

  missionParams.Parse(missionParamsName, NULL, &local, &globals);

  // addons
  ConstParamEntryPtr addons = missionParams.FindEntry("addons");
  if (addons && addons->GetSize() > 0)
  {
    for (int i=0; i<addons->GetSize(); i++)
    {
      RString addon = (*addons)[i];
      addOns.AddUnique(addon);
    }
    // check presence of addons
    bool CheckMissingAddons(FindArrayRStringCI &addOns);
    if (!CheckMissingAddons(addOns))
    {
      return false;
    }
  }
  return true;
}



///////////////////////////////////////////////////////////////////////////////
// class AI

static const EnumName SemaphoreNames[]=
{
  EnumName(-1, "NO CHANGE", &IDS_NO_CHANGE),
  EnumName(AI::SemaphoreBlue, "BLUE", &IDS_IGNORE),
  EnumName(AI::SemaphoreGreen, "GREEN", &IDS_BYPASS),
  EnumName(AI::SemaphoreWhite, "WHITE", &IDS_WHITE),
  EnumName(AI::SemaphoreYellow, "YELLOW", &IDS_CONTACT),
  EnumName(AI::SemaphoreRed, "RED", &IDS_ENGAGE),
  EnumName()
};
template<>
const EnumName *GetEnumNames(AI::Semaphore dummy)
{
  return SemaphoreNames;
}

static const EnumName RankNames[]=
{
  EnumName(RankUndefined, "UNDEFINED", &DUPLICATE_IDS),
  EnumName(RankPrivate, "PRIVATE", &IDS_PRIVATE),
  EnumName(RankCorporal, "CORPORAL", &IDS_CORPORAL),
  EnumName(RankSergeant, "SERGEANT", &IDS_SERGEANT),
  EnumName(RankLieutenant, "LIEUTENANT", &IDS_LIEUTENANT),
  #if !_ENABLE_INDEPENDENT_AGENTS
  EnumName(RankLieutenant, "LIEUTNANT", &DUPLICATE_IDS), // backward compatibility - old missions contain typo
  #endif
  EnumName(RankCaptain, "CAPTAIN", &IDS_CAPTAIN),
  EnumName(RankMajor, "MAJOR", &IDS_MAJOR),
  EnumName(RankColonel, "COLONEL", &IDS_COLONEL),
  EnumName()
};
template<>
const EnumName *GetEnumNames(Rank dummy)
{
  return RankNames;
}

AITime AI::GetActualTime()
{
  // static member function
  float time=Glob.clock.GetTimeInYear();
  // convert in-day time and in-year time to single int represetation
  const float secPerYear=365*24*60*60;
  return toIntFloor(secPerYear*time);
}

float AI::ExpForRank(Rank rank, bool ingame)
{
  if (rank < RankPrivate)
    return 0;

  if (rank > RankColonel)
    return FLT_MAX;

  if (ingame)
    return 0.8 * ExperienceTable[rank];

  return ExperienceTable[rank];
}

Rank AI::RankFromExp(float exp, bool ingame)
{
  int i;
  for (i=RankPrivate+1; i<=RankColonel; i++)
    if (exp < ExpForRank((Rank)i, ingame))
      return (Rank)(i - 1);
  return RankColonel;
}

AutoArray<float> ExperienceTable;
AutoArray<ExperienceDestroyInfo> ExperienceDestroyTable;

float ExperienceDestroyEnemy;
float ExperienceDestroyFriendly;
float ExperienceDestroyCivilian;
float ExperienceDestroyStatic;

float ExperienceKilled;

float ExperienceRenegadeLimit;
  
// for subordinate soldier only
float ExperienceCommandCompleted;
float ExperienceCommandFailed;
float ExperienceFollowMe;

// for leadership only
float ExperienceDestroyYourUnit; 
float ExperienceMissionCompleted;
float ExperienceMissionFailed;

void AI::InitTables()
{
  ParamEntryVal cls = Pars >> "CfgExperience";
  
  int n = (cls >> "ranks").GetSize(); 
  Assert(n == NRanks);

  ExperienceTable.Resize(n);
  for (int i=0; i<n; i++)
  {
    ExperienceTable[i] = (cls >> "ranks")[i];
  }

  n = (cls >> "destroyUnit").GetSize();
  ExperienceDestroyTable.Resize(n);
  for (int i=0; i<n; i++)
  {
    RString name = (cls >> "destroyUnit")[i];
    ExperienceDestroyTable[i].maxCost = (cls >> name)[0];
    ExperienceDestroyTable[i].exp = (cls >> name)[1];
  }

  ExperienceDestroyEnemy = cls >> "destroyEnemy";
  ExperienceDestroyFriendly = cls >> "destroyFriendly";
  ExperienceDestroyCivilian = cls >> "destroyCivilian";
  ExperienceDestroyStatic = cls >> "destroyStatic";

  ExperienceRenegadeLimit = cls >> "renegadeLimit";


  ExperienceKilled = cls >> "playerKilled";
    
  // for subordinate soldier only
  ExperienceCommandCompleted = cls >> "commandCompleted";
  ExperienceCommandFailed = cls >> "commandFailed";
  ExperienceFollowMe = cls >> "followMe";

  // for leadership only
  ExperienceDestroyYourUnit = cls >> "destroyYourUnit"; 
  ExperienceMissionCompleted = cls >> "missionCompleted";
  ExperienceMissionFailed = cls >> "missionFailed";
}

AIStatsMission::AIStatsMission()
{
}

void AIStatsMission::Init()
{
}

/*!
\patch 1.11 Date 07/30/2001 by Jirka
- Fixed: score does not work if squad id entered
\patch_internal 1.11 Date 07/30/2001 by Jirka
- different names used in initialization of table and searching in table
*/
void AIStatsMission::Clear()
{
  _start = Glob.time;
}


Person *GetPlayerPerson(EntityAI *vehicle)
{
  if (!vehicle) return NULL;
  Person *person = dyn_cast<Person>(vehicle);
  if (person) return person;
  Transport *trans = dyn_cast<Transport>(vehicle);
  if (!trans) return NULL; // may be Thing or Building

  // check if player is inside
  Person *commander = trans->Observer();
  if (commander && commander->IsNetworkPlayer()) return commander;
  Person *gunner = trans->Gunner();
  if (gunner && gunner->IsNetworkPlayer()) return gunner;
  Person *driver = trans->Driver();
  if (driver && driver->IsNetworkPlayer()) return driver;

  // check if anybody is inside
  if (commander) return commander;
  if (gunner) return gunner;
  return driver;
}

#if _ENABLE_VBS
/*!
\patch_internal 1.78 Date 7/16/2002 by Jirka
- Added: Multiplayer report for VBS
*/

void AIStatsMission::VBSKilled(EntityAI *killed, EntityAI *killer)
{
  Assert(killed);
  Assert(killer);
  
  Person *killedPerson = GetPlayerPerson(killed);
  if (!killedPerson) return;
  Person *killerPerson = GetPlayerPerson(killer);
  if (!killerPerson) return;

  int index = _vbsEvents.Add();
  VBSStatsEvent &event = _vbsEvents[index];
  
  event.type = VBSETKill;
  event.t = Glob.time;

  // killed
  event.unit1.name = killedPerson->GetPersonName();
  event.unit1.type = killed->GetType()->GetName();
  event.unit1.position = killed->FutureVisualState().Position();
  event.unit1.side = killedPerson->Vehicle::GetTargetSide();
  event.unit1.player = killedPerson->IsNetworkPlayer();

  // killer
  event.unit2.name = killerPerson->GetPersonName();
  event.unit2.type = killer->GetType()->GetName();
  event.unit2.position = killer->FutureVisualState().Position();
  event.unit2.side = killerPerson->Vehicle::GetTargetSide();
  event.unit2.player = killerPerson->IsNetworkPlayer();
}

void AIStatsMission::VBSInjured(EntityAI *injured, EntityAI *killer, float damage, RString ammo)
{
  Assert(injured);
  Person *person = GetPlayerPerson(injured);
  if (!person) return;
  Person *killerPerson = GetPlayerPerson(killer);
  if (!killerPerson) return;

  int index = _vbsEvents.Add();
  VBSStatsEvent &event = _vbsEvents[index];
  
  event.type = VBSETInjury;
  event.t = Glob.time;

  // injured
  event.unit1.name = person->GetPersonName();
  event.unit1.type = injured->GetType()->GetName();
  event.unit1.position = injured->FutureVisualState().Position();
  event.unit1.side = person->Vehicle::GetTargetSide();
  event.unit1.player = person->IsNetworkPlayer();

  // killer
  event.unit2.name = killerPerson->GetPersonName();
  event.unit2.type = killer->GetType()->GetName();
  event.unit2.position = killer->FutureVisualState().Position();
  event.unit2.side = killerPerson->Vehicle::GetTargetSide();
  event.unit2.player = killerPerson->IsNetworkPlayer();

  event.value = damage;
  event.text = ammo;
}

void AIStatsMission::VBSAddHeader(RString text)
{
  _vbsHeader.Add(text);
}

void AIStatsMission::VBSAddEvent(RString text)
{
  int index = _vbsEvents.Add();
  VBSStatsEvent &event = _vbsEvents[index];
  
  event.type = VBSETUser;
  event.t = Glob.time;
  event.text = text;
}

void AIStatsMission::VBSAddFooter(RString text)
{
  _vbsFooter.Add(text);
}

#include <Es/Common/win.h>

void ReportOutput(HANDLE report, const char *text)
{
  DWORD cbWritten;
  WriteFile(report, text, strlen(text), &cbWritten, 0);
  WriteFile(report, "\r\n", 2, &cbWritten, 0);
}

void AIStatsMission::WriteVBSStatistics()
{
  HANDLE report = ::CreateFile
  (
    "mpreport.txt",
    GENERIC_WRITE,
    0,
    0,
    OPEN_ALWAYS, //CREATE_ALWAYS,
    FILE_FLAG_WRITE_THROUGH,
    0
  );
  if (!report) return;
  
  SetFilePointer(report, 0, 0, FILE_END);

  BString<512> buffer;

  // Header
  ReportOutput(report, "----------**Start**----------");
  sprintf
  (
    buffer, "Mission: %s.%s",
    (const char *)Glob.header.filenameReal, (const char *)Glob.header.worldname
  );
  ReportOutput(report, buffer);
  for (int i=0; i<_vbsFooter.Size(); i++) ReportOutput(report, _vbsFooter[i]);

  RString sides, players;
  for (int i=0; i<TSideUnknown; i++)
  {
    int PlayersNumber(TargetSide side);
    int count = PlayersNumber((TargetSide)i);
    if (count == 0) continue;
    
    if (sides.GetLength() > 0) sides = sides + RString("/");
    if (players.GetLength() > 0) players = players + RString("/");

    switch (i)
    {
    case TEast:
      sides = sides + LocalizeString(IDS_EAST); break;
    case TWest:
      sides = sides + LocalizeString(IDS_WEST); break;
    case TGuerrila:
      sides = sides + LocalizeString(IDS_GUERRILA); break;
    case TCivilian:
      sides = sides + LocalizeString(IDS_CIVILIAN); break;
    }
    
    players = players + FormatNumber(count);
  }
  sprintf
  (
    buffer, "Players: %s (%s)",
    (const char *)sides, (const char *)players
  );
  ReportOutput(report, buffer);
  
  struct tm *time = localtime(&_time);
  sprintf
  (
    buffer, "Start mission: %d/%d/%d, %d:%02d:%02d",
    time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
    time->tm_hour, time->tm_min, time->tm_sec
  );
  ReportOutput(report, buffer);

  for (int i=0; i<_vbsHeader.Size(); i++) ReportOutput(report, _vbsHeader[i]);

  // Events
  ReportOutput(report, "----------**Events**----------");
  int totalKills[TSideUnknown], totalKilled[TSideUnknown];
  for (int i=0; i<TSideUnknown; i++)
  {
    totalKills[i] = 0; totalKilled[i] = 0;
  }
  for (int i=0; i<_vbsEvents.Size(); i++)
  {
    VBSStatsEvent &event = _vbsEvents[i];

    int time = toInt(event.t.toFloat());
    int h = time / 3600;
    time -= 3600 * h;
    int m = time / 60;
    time -= 60 * m;
    int s = time;
    
    switch (event.type)
    {
    case VBSETKill:
      {
        sprintf
        (
          buffer, "%2d:%02d:%02d : %s%s killed by %s%s at [%.0f, %.0f]",
          h, m, s,
          (const char *)event.unit1.name, event.unit1.player ? "" : " (AI)",
          (const char *)event.unit2.name, event.unit2.player ? "" : " (AI)",
          event.unit1.position.X(), event.unit1.position.Z()
        );
        ReportOutput(report, buffer);
        totalKills[event.unit2.side]++;
        totalKilled[event.unit1.side]++;
      }
      break;
    case VBSETInjury:
      {
        BString<256> ammo;
        if (event.text.GetLength() > 0) sprintf(ammo, ", ammo %s", (const char *)event.text);
        sprintf
        (
          buffer, "%2d:%02d:%02d : %s%s injured by %s%s at [%.0f, %.0f], damage %.0f%%%s",
          h, m, s,
          (const char *)event.unit1.name, event.unit1.player ? "" : " (AI)",
          (const char *)event.unit2.name, event.unit2.player ? "" : " (AI)",
          event.unit1.position.X(), event.unit1.position.Z(),
          100.0f * event.value, (const char *)ammo
        );
        ReportOutput(report, buffer);
      }
      break;
    case VBSETUser:
      {
        sprintf
        (
          buffer, "%2d:%02d:%02d : %s",
          h, m, s,
          (const char *)event.text
        );
        ReportOutput(report, buffer);
      }
      break;
    }
  }

  // Footer
  ReportOutput(report, "----------**Summary**----------");
  RString kills, killed;
  for (int i=0; i<TSideUnknown; i++)
  {
    int PlayersNumber(TargetSide side);
    int count = PlayersNumber((TargetSide)i);
    if (count == 0) continue;
    
    if (kills.GetLength() > 0) kills = kills + RString("/");
    if (killed.GetLength() > 0) killed = killed + RString("/");

    kills = kills + FormatNumber(totalKills[i]);
    killed = killed + FormatNumber(totalKilled[i]);
  }
  sprintf
  (
    buffer, "Total amount of deaths: %s (%s)",
    (const char *)sides, (const char *)killed
  );
  ReportOutput(report, buffer);
  sprintf
  (
    buffer, "Total amount of kills: %s (%s)",
    (const char *)sides, (const char *)kills
  );
  ReportOutput(report, buffer);
  for (int i=0; i<_vbsHeader.Size(); i++) ReportOutput(report, _vbsFooter[i]);

  ReportOutput(report, "----------**End**----------");
  ReportOutput(report, "");

  CloseHandle(report);
}

#endif

void AIStatsMission::OnMPMissionStart()
{
  time(&_time);
}


LSError AIStatsMission::Serialize(ParamArchive &ar)
{
  CHECK(::Serialize(ar, "start", _start, 1))
  return LSOK;
}

LSError AIUnitHeader::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("index", index, 1))
  CHECK(ar.Serialize("experience", experience, 1, 0))
  CHECK(ar.SerializeEnum("rank", rank, 1, RankUndefined))
  CHECK(ar.SerializeRef("Unit", unit, 1))
  CHECK(ar.Serialize("used", used, 1, false))
  return LSOK;
}

void PositionToAA11(Vector3Val pos, char *buffer);

AIStats::AIStats()
{
}
AIStats::~AIStats()
{
}

void AIStats::OnMPMissionEnd()
{
}

void AIStats::OnMPMissionStart()
{
  _mission.OnMPMissionStart();
}

void AIStats::OnVehicleDestroyed(EntityAI *killed, EntityAI *killer)
{
 
}

LSError AIStats::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("Mission", _mission, 1))
  return LSOK;
}

AIStats GStats;
