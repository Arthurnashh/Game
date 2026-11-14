#ifndef _SVN_AUTO_UPDATER_H
#define _SVN_AUTO_UPDATER_H

#include "pipeManager.h"

class SVNAutoUpdaterClient 
{
public:
  SVNAutoUpdaterClient() : _ready(false)
  {
    for (int i=0; i<sizeof(_pipes)/sizeof(_pipes[0]); i++)
    {
      _pipes[i] = -1;
    }
  }
  ~SVNAutoUpdaterClient() 
  {
    _pipeManager.Send(PipeManager::CmdClosing);

    // close my pipe ends
    if (_pipes[0]!=-1) close(_pipes[0]);
    if (_pipes[3]!=-1) close(_pipes[3]);
  }

  bool Update();

  // returns true when initialization was OK
  bool Init(const char *svnAUPath, const char *svnURL, const char *user, const char *passwd);

private:
  // Get my side in/out file descriptors
  int PipeIn() { return _pipes[0]; }
  int PipeOut() { return _pipes[3]; }

  // pipe to communicate with SvnAutoUpdater (server)
  PipeManager _pipeManager;
  int _pipes[4];
  HANDLE hProcess;
  bool  _ready;
};

#endif
