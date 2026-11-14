#if defined(_WIN32) 

#include <El/elementpch.hpp>
#include "fileBinMake.hpp"
#include <direct.h>
#include <process.h>
#include <errno.h>
//#include <Es/Files/filenames.hpp>
#include <Es/Strings/bString.hpp>
#include <Es/Common/win.h>

/**
@file
Provides an interface how to call a binMake
to make target file from a source file.
*/

#define ConfigApp "Software\\BIStudio\\BinMake"

static RString GetRegPath(RString &reg, const char *value)
{
	reg = "HKEY_LOCAL_MACHINE/" ConfigApp;
	HKEY key;
	if
	(
		!::RegOpenKeyEx
		(
			HKEY_LOCAL_MACHINE, ConfigApp,
			0, KEY_READ, &key
		) == ERROR_SUCCESS
	) return RString();

	RString ret;
	DWORD type = REG_SZ;
	char buffer[1024];
	DWORD size = sizeof(buffer);
	HRESULT hr = ::RegQueryValueEx(key, value, 0, &type, (BYTE *)buffer, &size);
	if (SUCCEEDED(hr))
	{
		ret = buffer;
	}
	else
	{
	  reg = reg + RString("/")+RString(value);
	}
	::RegCloseKey(key);
	return ret;
}

int BinMakeFunctions::Make(const char *src, const char *tgt, const char *rules, BinMakeHandle *handle)
{
  RString reg;
  // read application/rules location from the registry
  // TODO: read rules name
  RString binDir = GetRegPath(reg,"path");
  if (binDir.GetLength()<=0)
  {
    fprintf(stderr,"Cannot read registry %s",cc_cast(reg));
    return -1;
  }
  RString binName = GetRegPath(reg,"exe");
  if (binName.GetLength()<=0)
  {
    fprintf(stderr,"Cannot read registry %s",cc_cast(reg));
    return -1;
  }

  // Get the run directory
  BString<1024> runDir;
  {
    char oldDir[1024];
    getcwd(oldDir,sizeof(oldDir));
    strcpy(runDir,oldDir);
  }

  // Compose the argument string
  char cmdLine[2048];
  sprintf(cmdLine, "\"%s%s\" -C \"%s\" \"%s\" \"%s\"", cc_cast(binDir), cc_cast(binName), cc_cast(runDir), src, tgt);
  //RString cmdLine = Format();
  //sprintf(cmdLine, "\"%s%s\" %s \"%s\" \"%s\"", cc_cast(binDir), cc_cast(binName), cc_cast(runDir), src, tgt);
  
  // Run the new process
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));
  BOOL cpResult = CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, binDir, &si, &pi);

  // Handle the results
  if (cpResult)
  {
    // Close the thread handle - we don't need it
    CloseHandle(pi.hThread);

    // Decide upon handle pointer whether handle should be closed or returned
    if (handle)
    {
      // Return the process
      *handle = (BinMakeHandle)pi.hProcess;
    }
    else
    {
      // Wait until the process finishes
      WaitForSingleObject(pi.hProcess, INFINITE);

      // Close the handle
      CloseHandle(pi.hProcess);
    }

    // Success
    return 0;
  }
  else
  {
    int lastError = GetLastError();
    fprintf(stderr,"Error: %d Cannot run %s\n", lastError, cc_cast(binName));

    // Fail
    return -1;
  }
}

#endif
