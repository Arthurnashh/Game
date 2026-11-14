#include "Es\Common\win.h"
#include "StdAfx.h"
#include "gameDirs.hpp"

RString GetLoginName()
{
  char buf[256];
  DWORD bufSize = sizeof(buf);
  if (::GetUserNameA(buf, &bufSize) && bufSize > 0) return buf;
  return "Player";
}

#include <shlobj.h>
#include <io.h>
#include <Es/Files/filenames.hpp>
#include <El/QStream/qStream.hpp>

/// command line may be used to override user profile storage area 
/* does not terminate with backslash */

static RString GetRootDir()
{
  char path[MAX_PATH];
  if (FAILED(SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, path)))
  {
    Fail("Cannot access My Documents");
    return RString();
  }
  TerminateBy(path, '\\');
  return path;
}

// these two should be defined somewhere
extern const AutoArray<RString> &GetProfilePathDirs();
extern RString GetAppName();
bool GetLocalSettingsDir(char *dir)
{
  RString appName = GetAppName();
  if (appName.IsEmpty()) return false; 
  if (FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, dir)))
  {
    Fail("Cannot access Local settings Application data");
    return false;
  }
  TerminateBy(dir, '\\');
  strcat(dir, appName);
  return true;
}

AutoArray<RString> GetDefaultUserRootDirs()
{
  AutoArray<RString> out;
  const AutoArray<RString> &dirs = GetProfilePathDirs();
  if (dirs.Size()==0) return out;
  RString root = GetRootDir();
  if (root.IsEmpty()) return out;
  for (int i=0; i<dirs.Size(); i++)
  {
    out.Add(root + dirs[i]);
  }
  return out;
}
