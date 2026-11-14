#pragma once
#include <afxwin.h>

class CTexMergeCmdLine : public CCommandLineInfo
{
  bool _server;
public:
  CTexMergeCmdLine():_server(false) {};
  void ParseParam(const char * pszParam, BOOL bFlag, BOOL bLast);

  bool IsServerRequested() {return _server;}

};
