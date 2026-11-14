#pragma once
#include "rocheck.h"

class TexMergeROCheck :
  public WinROCheck
{
  bool _serverMode;
public:
  TexMergeROCheck(void):_serverMode(false) {}  

  void SetServerMode(bool serverMode) {_serverMode=serverMode;}
  bool IsServerMode() {return _serverMode;}

  virtual ROCheckResult TestFileRO(const _TCHAR *szFilename, int flags);
};
