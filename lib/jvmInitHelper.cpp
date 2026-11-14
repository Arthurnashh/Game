#include "wpch.hpp"
#include <shlobj.h>
#include <Es/Strings/rString.hpp>

RString GetWinFontsDirectory()
{
  char path[MAX_PATH];
  if (SHGetSpecialFolderPath(NULL, path, CSIDL_FONTS, FALSE)) return path;
  return RString();
}