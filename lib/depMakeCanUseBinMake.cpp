#include "wpch.hpp"
#include "depMake.hpp"
#include <El/Interfaces/iFileMake.hpp>

static FileMakeFunctions SMakeFunctionsDefault;
FileMakeFunctions *GFileMakeFunctions = &SMakeFunctionsDefault;

#if _ENABLE_BULDOZER

#include "engine.hpp"
#include "textbank.hpp"
#include <El/FileBinMake/fileBinMake.hpp>
#include <Es/Files/filenames.hpp>
#include <El/QStream/qbStream.hpp>
#include <El/Debugging/debugTrap.hpp>

static BinMakeFunctions SBinMakeFunctions;

void SetBinMakeFunctions()
{
  GFileMakeFunctions = &SBinMakeFunctions;
}

extern bool LandEditor;

#endif

RString GlobResolveTexture(RString name)
{
#if _ENABLE_BULDOZER
  if (LandEditor)
  {
    const char *ext = GetFileExt(GetFilenameExt(name));
    if (!strcmp(ext,".tga") || !strcmp(ext,".png"))
    {
      Debugger::PauseCheckingScope scope(GDebugger);

      char tgtName[1024];
      strcpy(tgtName,name);
      char *tgtExt = GetFileExt(GetFilenameExt(tgtName));
      strcpy(tgtExt,".paa");
      // if the file is already up-to-date, do not make it
      QFileTime sTime = QFBankQueryFunctions::TimeStamp(name);
      QFileTime tTime = QFBankQueryFunctions::TimeStamp(tgtName);
      if (tTime>=sTime) return tgtName;
      // before making the file we need to release the texture
      AbstractTextBank::TextureSuspendScope texScope(GEngine->TextBank(),tgtName);
      if (!GFileServerFunctions->FlushReadHandle(tgtName))
      {
        // cannot write the target - it is locked
        return tgtName;
      }
      // check if we can make the file
      if (GFileMakeFunctions->Make(name,tgtName,"binarize")>=0)
      {
        return tgtName;
      }
      return "";
    }
  }
#endif
  return name;
}
