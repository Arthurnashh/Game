#include "StdAfx.h"
#include ".\texmergecmdline.h"

void CTexMergeCmdLine::ParseParam(const char * pszParam, BOOL bFlag, BOOL bLast)
{
  if (bFlag && stricmp(pszParam,"server")==0)
    _server=true;
  else 
    __super::ParseParam(pszParam,bFlag,bLast);
}
