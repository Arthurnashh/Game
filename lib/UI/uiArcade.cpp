// Implementation of mission editor dialogs
#include "../wpch.hpp"
#include "uiMap.hpp"
#include <Es/Common/win.h>
#include "../dikCodes.h"
#include "../Protect/selectProtection.h"

#include "resincl.hpp"
#include "../keyInput.hpp"
#include <El/QStream/qbStream.hpp>
#include <Es/Algorithms/qsort.hpp>
#include <El/ParamFile/classDbParamFile.hpp>
#include <El/ParamArchive/paramArchiveDb.hpp>

#include <El/Common/randomGen.hpp>
#include "../camEffects.hpp"
#include "../titEffects.hpp"
#include "../landscape.hpp"

#include <El/Evaluator/expressImpl.hpp>
#include "../house.hpp"

#include "../stringtableExt.hpp"
#include "../fileLocator.hpp"

#include "../saveVersion.hpp"

#include "missionDirs.hpp"
#include "../gameDirs.hpp"
#include "../saveGame.hpp"

RString GetVehicleIcon(RString name);

RStringB GetExtensionWizardMission();

bool HasPrimaryGunnerTurret(ParamEntryPar cls);
bool HasPrimaryObserverTurret(ParamEntryPar cls);

/*!
\file
Implementation file for particular displays (mission editor).
*/


//! returns current (topmost) display
Display *CurrentDisplay()
{
  if (!GWorld) return NULL;

  Display *disp = dynamic_cast<Display *>(GWorld->Options());
  if (!disp) return NULL;

  ControlsContainer *ptr = disp;
  while (ptr->Child())
    ptr = ptr->Child();
  return dynamic_cast<Display *>(ptr);
}

//! creates message box in topmost display (NOT CURRENTLY USED)
bool MsgBox(RString text, int flags, int idd)
{
  Display *disp = CurrentDisplay();
  if (!disp) return false;

  disp->CreateMsgBox(flags, text, idd);
  return true;
}

//! creates message box in topmost display (NOT CURRENTLY USED)
bool MsgBox(int ids, int flags, int idd)
{
  return MsgBox(LocalizeString(ids), flags, idd);
}

//! parse name of template (only file name) and sets current mission
bool ProcessTemplateName(RString name, RString dir)
{
  char buffer[256];
  strcpy(buffer, name);

  // parse mission name

  // mission
  char *world = strchr(buffer, '.');
  if (!world) return false;
  *world = 0;
  world++;

  // world
  char *ext = strchr(world, '.');
  if (ext)
  {
    *ext = 0;
    ext++;
    Assert(stricmp(ext, "sqm") == 0);
  }

  SetMission(world, buffer, dir);
  return true;
}

//! parse name of template (only file name) and sets current mission
bool ProcessTemplateName(RString name)
{
  return ProcessTemplateName(name, MissionsDir);
}

//! parse name of template (full path) and sets current base directory and mission
bool ProcessFullName(RString name)
{
  // parse filename
  char buffer[256];
  strcpy(buffer, name);
  strlwr(buffer);

  // filename
  char *ext = strrchr(buffer, '\\');
  if (!ext) return false;
  *ext = 0;

  // world
  ext = strrchr(buffer, '.');
  if (!ext) return false;
  Assert(ext);
  RString world = ext + 1;
  *ext = 0;

  // mission
  ext = strrchr(buffer, '\\');
  if (!ext) return false;

  RString mission = ext + 1;
  *(ext + 1) = 0;

  // base directory and subdirectory
  RString dir, subdir;
  const char *ptr = "campaigns\\";
  ext = strstr(buffer, ptr);
  if (ext)
  {
    ext = strchr(ext + strlen(ptr), '\\');
    if (!ext) return false;
    subdir = ext + 1;
    *(ext + 1) = 0;
    dir = buffer;
  }
  else subdir = buffer;

  CurrentMission = "";
  SetBaseDirectory(false, dir);
  SetMission(world, mission, subdir);
  return true;
}

extern AutoArray<char> GTransferredUserMission;

//! reads mission file (mission.sqm) and parse given cutscene / mission into CurrentTemplate
bool ParseCutscene(RString cutscene, bool multiplayer, bool avoidCheckIds, bool allowEmpty = false)
{
  SECUROM_MARKER_SECURITY_ON(14)
  FreeOnDemandGarbageCollect(1024*1024,256*1024);
  
  bool isPartMission = stricmp(cutscene, "Mission") == 0;

  RString name;
  RString sqfName = GetMissionDirectory() + RString("mission.sqf");
  if (QFBankQueryFunctions::FileExists(sqfName))
  {
    CurrentTemplate = new ScriptedMission();
    name = sqfName;
  }
  else
  {
    CurrentTemplate = new ArcadeTemplate();
    name = GetMissionDirectory() + RString("mission.sqm");
  }

  GameVarSpace local(false);
  int gshandle = GGameState.BeginContext(&local);

  GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

  RString missionParamsName = GetMissionParameters();
  bool hasMissionParams = missionParamsName.GetLength() > 0;
  if (hasMissionParams)
  {

    // mission parameters file
    ParamFile missionParams;

    missionParams.Parse(missionParamsName, NULL, NULL, &globals);

    // parameters
    float weather = missionParams >> "weather";
    GGameState.VarSetLocal("_weather", weather);
    float weatherForecast = missionParams >> "weatherForecast";
    GGameState.VarSetLocal("_weatherForecast", weatherForecast);
    float fog = missionParams >> "fog";
    GGameState.VarSetLocal("_fog", fog);
    float fogForecast = missionParams >> "fogForecast";
    GGameState.VarSetLocal("_fogForecast", fogForecast);
    float viewDistance = 900.0f;
    ConstParamEntryPtr entry = missionParams.FindEntry("viewDistance");
    if (entry) viewDistance = *entry;
    GGameState.VarSetLocal("_viewDistance", viewDistance);
    int year = missionParams >> "year";
    GGameState.VarSetLocal("_year", (float)year);
    int month = missionParams >> "month";
    GGameState.VarSetLocal("_month", (float)month);
    int day = missionParams >> "day";
    GGameState.VarSetLocal("_day", (float)day);
    int hour = missionParams >> "hour";
    GGameState.VarSetLocal("_hour", (float)hour);
    int minute = missionParams >> "minute";
    GGameState.VarSetLocal("_minute", (float)minute);

    // positions
    ConstParamEntryPtr parPos = missionParams.FindEntry("Positions");
    if (parPos)
    {
      for (int i=0; i<parPos->GetEntryCount(); i++)
      {
        ParamEntryVal entry = parPos->GetEntry(i);
        RString name = entry.GetName();
        float x = entry[0];
        float y = entry[1];
        float z = entry[2];
        GGameState.VarSetLocal(RString("_") + name + RString("_X"), x);
        GGameState.VarSetLocal(RString("_") + name + RString("_Y"), y);
        GGameState.VarSetLocal(RString("_") + name + RString("_Z"), z);
      }
    }

    // parameters
    ConstParamEntryPtr parParams = missionParams.FindEntry("Params");
    if (parParams)
    {
      for (int i=0; i<parParams->GetEntryCount(); i++)
      {
        ParamEntryVal entry = parParams->GetEntry(i);
        RString name = entry.GetName();
        float value = missionParams >> "Params" >> name; 

        GGameState.VarSetLocal(RString("_") + name, value);
      }
    }
  }

  {
    if (!CurrentTemplate->Load(name,cutscene,globals,local))
    {
      GGameState.EndContext(gshandle);
      return false;
    }


  }

  if (hasMissionParams && isPartMission)
  {
    if (!CurrentTemplate->ParseMissionParams(missionParamsName,local))
    {
      GGameState.EndContext(gshandle);
      CurrentTemplate = new DummyMission;
      return false;

    }

/*
    // Testing save
    ATSParams params;
    params.avoidCheckSync = true;
    ParamArchiveSave out(11, &params);
    CurrentTemplate->Serialize(out);
    out.Save("test.sqm");
*/
  }

  GGameState.EndContext(gshandle);
  
  FreeOnDemandGarbageCollect(1024*1024,256*1024);

  Glob.header.playerSide = TCivilian; //Cutscene
  SECUROM_MARKER_SECURITY_OFF(14)

  return true;
}

//! reads mission file (mission.sqm) and parse intro cutscene into CurrentTemplate
bool ParseIntro(bool avoidCheckIds)
{
  return ParseCutscene("Intro", false, avoidCheckIds, true);
}

//! reads mission file (mission.sqm) and parse mission into CurrentTemplate
bool ParseMission(bool multiplayer, bool avoidCheckIds, bool allowEmpty = false)
{
  return ParseCutscene("Mission", multiplayer, avoidCheckIds, allowEmpty);
}

