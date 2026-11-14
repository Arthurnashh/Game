#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MISSION_DIRS_HPP
#define _MISSION_DIRS_HPP

#include <Es/Strings/rString.hpp>

RString GetUserDirectory(RString name);
RString GetUserDirectory();

RString GetMissionParameters();
RString GetMissionParametersDisplayName();
RString GetBaseDirectoryRaw();
RString GetBaseDirectory();
RString GetBaseSubdirectory();
RString GetMissionsDirectory();
RString GetMissionDirectory();
bool IsUserMission();

void SetMissionParameters(RString params, RString displayName);
void SetBaseDirectory(bool userMission, RString dir, bool cleanup = true);
void SetBaseSubdirectory(RString dir);
void SetMission(RString world, RString mission, RString subdir, RString params, RString displayName, bool cleanup = true);
void SetMission(RString world, RString mission, RString subdir, bool cleanup = true);
void SetMission(RString world, RString mission, bool cleanup = true);

#if _ENABLE_MISSION_CONFIG
class ParamEntry;

bool SelectMission(const ParamEntry &cfg);
#endif

extern RString MPMissionsDir;
extern RString AnimsDir;
extern RString MissionsDir;

class GameDataNamespace;
/// namespace used for parsing of configs
extern GameDataNamespace GParsingNamespace;

#endif
