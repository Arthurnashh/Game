#include ".\texmergerocheck.h"
#include <io.h>

ROCheckResult TexMergeROCheck::TestFileRO(const _TCHAR *szFilename, int flags)
{
  if (_serverMode)
  {
    if (_access(szFilename,02)==0) return ROCHK_FileOK;
    if (_access(szFilename,00)==0) return ROCHK_FileRO;
    return ROCHK_FileOK;
  }
  else
    return WinROCheck::TestFileRO(szFilename,flags);
}