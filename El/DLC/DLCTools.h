#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DLC_TOOLS_H
#define _DLC_TOOLS_H

#if _ENABLE_DLC_SUPPORT

#include <Es/Strings/rString.hpp>

namespace DLCTools
{
  // Checks if there is updated version of any DLC in DLC setup directory and runs setup.exe in order to install updated version in game directory
  void CheckDLCForUpdate();

  // Returns true is setup should be executed in order to install, update or repair DLC installation
  bool IsCheckNeeded(RString setupDir);

  // Reads version from setupVersion file and store it in gameVersion file (called when DLC update is complete)
  void SaveCurrentVersionFile(RString setupDir);
}

#endif //_ENABLE_DLC_SUPPORT

#endif //_DLC_TOOLS_H
