#include "../wpch.hpp"
#include "missionDirs.hpp"

#include <El/Stringtable/stringtable.hpp>

#include "../paramFileExt.hpp"
#include "../global.hpp"
#include "../landscape.hpp"
#include "../world.hpp"
#include "../gameDirs.hpp"

/// Flag deciding whether we are playing the user mission
static bool UserMission;

//! base directory
/*!
Possible values:  
- base application directory for single missions or multiplayer missions
*/
static RString BaseDirectory;
//! base subdirectory
/*!
Possible values:
- Missions for singleplayer missions
- MPMissions for multiplayer missions
- Anims for animations
*/
static RString BaseSubdirectory;

//! parameters file for template based user missions
static RString MissionParameters;

//! display name of the template based user missions
static RString MissionParametersDisplayName;

//! returns path of user directory for given user
RString GetUserDirectory(RString name)
{
  RString dir = GetUserRootDir(name);
  if (dir.GetLength() == 0) return RString();
  return dir + RString("\\");
}

//! returns path of user directory for current user
RString GetUserDirectory()
{
  return GetUserDirectory(Glob.header.GetPlayerName());
}

//! return name of parameters file for template based user missions
RString GetMissionParameters() {return MissionParameters;}

//! return display name of the template based user missions
RString GetMissionParametersDisplayName() {return MissionParametersDisplayName;}

//! returns value of BaseDirectory variable (used for serialization only)
RString GetBaseDirectoryRaw()
{
  return BaseDirectory;
}

//! returns base directory
RString GetBaseDirectory()
{
  if (UserMission)
    return GetUserDirectory() + BaseDirectory;
  else
    return BaseDirectory;
}

//! returns base subdirectory
RString GetBaseSubdirectory() {return BaseSubdirectory;}

//! returns current directory for missions
RString GetMissionsDirectory()
{
  return GetBaseDirectory() + GetBaseSubdirectory();
}

//! returns directory of current mission
RString GetMissionDirectory()
{
  if (Glob.header.filename[0]==0)
  {
    return RString();
  }
  if (GetMissionParameters().GetLength() == 0 || stricmp(Glob.header.filename, "__cur_mp") == 0 || stricmp(Glob.header.filename, "__cur_sp") == 0)
    return GetMissionsDirectory() +
    RString(Glob.header.filename) + RString(".") +
    RString(Glob.header.worldname) + RString("\\");
  else
    return GetMissionsDirectory() +
    RString(Glob.header.filename) + RString("\\");
}

bool IsUserMission()
{
  return UserMission;
}

void SetMissionParameters(RString params, RString displayName)
{
  MissionParameters = params;
  MissionParametersDisplayName = displayName;
}

//! sets base directory
void SetBaseDirectory(bool userMission, RString dir, bool cleanup)
{
  if (cleanup)
  {
    if (GWorld) GWorld->CleanUp();
    if (GLandscape) GLandscape->ResetState();
    GWorld->GetMissionNamespace()->Reset();
  }

  UserMission = userMission;
  BaseDirectory = dir;
}

//! sets base subdirectory
void SetBaseSubdirectory(RString dir)
{
  BaseSubdirectory = dir;
}

RString MPMissionsDir("mpmissions\\");
RString AnimsDir("anims\\");
RString MissionsDir("missions\\");

//! sets current mission
/*!
Also reads new mission stringtable and mission description.ext file.
*/
void SetMission(RString world, RString mission, RString subdir, RString params, RString displayName, bool cleanup)
{
  if (GWorld) GWorld->OnFinishMission();
  
  //Checks if HASP key is inserted, see Trial.hpp
#if _VBS3 && _TIME_EXPIRY
  bool CheckIfKeyIsInserted();
  if(!CheckIfKeyIsInserted())
    ErrorMessage("HASP key was removed!");
#endif

  MissionParameters = params;
  MissionParametersDisplayName = displayName;

  strcpy(Glob.header.worldname, world);
  strcpy(Glob.header.filename, mission);
  Glob.header.filenameReal = mission;

  // remove possible references to the ExtParsMission
  if (cleanup)
  {
    if (GWorld) GWorld->CleanUp();
    if (GLandscape) GLandscape->ResetState();
    GWorld->GetMissionNamespace()->Reset();
  }

  BaseSubdirectory = subdir;
  ExtParsMission.Clear();

  RString dir = GetMissionDirectory();
  if (dir.GetLength() > 0)
  {
    // mission stringtable
    RString filename = GetMissionDirectory() + RString("stringtable.csv");
    LoadStringtable("Mission", filename, 2, true);
    // mission description
    filename = GetMissionDirectory() + RString("description.ext");
    if (QFBankQueryFunctions::FileExists(filename))
      ExtParsMission.Parse(filename, NULL, NULL, &GParsingNamespace); // parsing namespace (persistent namespace needed for persistent ParamFile)
  }
}

//! sets current mission (used for singleplayer missions only)
void SetMission(RString world, RString mission, RString subdir, bool cleanup)
{
  SetMission(world, mission, subdir, RString(), RString(), cleanup);
}

//! sets current mission (used for singleplayer missions only)
void SetMission(RString world, RString mission, bool cleanup)
{
  SetMission(world, mission, MissionsDir, RString(), RString(), cleanup);
}

#if _ENABLE_MISSION_CONFIG
bool SelectMission(const ParamEntry &cfg)
{
  RString dir = cfg >> "directory";
  const char *ptr1 = dir;
  const char *ptr2 = strrchr(ptr1, '\\');
  if (!ptr2) return false;
  const char *ptr3 = strrchr(ptr2 + 1, '.');
  if (!ptr3) return false;

  RString subdir(ptr1, ptr2 + 1 - ptr1);
  RString mission(ptr2 + 1, ptr3 - (ptr2 + 1));
  RString world(ptr3 + 1);
  RString displayName;
  if ( cfg.FindEntry("briefingName") ) displayName = cfg >> "briefingName";
  SetMission(world, mission, subdir, RString(), displayName);
  return true;
}
#endif

/// namespace used for parsing of configs
GameDataNamespace GParsingNamespace(NULL, "parsing", false);

#include <El/Modules/modules.hpp>

INIT_MODULE(MissionDirs, 2)
{
  // namespace need to be registered manually because of order of static variables initialization
  GGameState.RegisterStaticNamespace(&GParsingNamespace);
}