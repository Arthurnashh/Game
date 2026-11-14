#ifndef _PIPE_MANAGER_H
#define _PIPE_MANAGER_H

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <io.h>
#include <iostream>
#include <fstream>

#include <queue>
#include <string>
using namespace std;

class PipeManager
{
public:
  //! Destructor sends the CmdClosing command before destruction
  static const char *CmdClosing;
  static const char *CmdFailed;

  PipeManager() : _hIn(INVALID_HANDLE_VALUE) {}

  //! pipe file descriptors are given by commandline
  PipeManager(int fdIn, int fdOut)
  {
    Init(fdIn, fdOut);
  }

  void Init(int fdIn, int fdOut)
  {
    _fdIn = fdIn; 
    _fdOut = fdOut; 
    if (fdIn!=-1 && fdOut!=-1)
      _hIn = (HANDLE)_get_osfhandle(fdIn);
    else
      _hIn = INVALID_HANDLE_VALUE;
  }
  //! read possible new bytes from fdIn and parse them into _commands
  void Update();
  //! get the command from queue if any
  string Receive();
  //! send the command
  void Send(string command);
  //! test whether pipe initialization succeeded
  bool Ready() const { return _hIn!=INVALID_HANDLE_VALUE; }  

private:
  //! file descriptors for read/write operations
  int     _fdIn, _fdOut;
  HANDLE  _hIn;
  //! buffer containing char received from fdIn but not processed yet
  string _buf;
  //! list of commands between <...> received from fdIn. See Update()
  queue<string> _commands;
};

#endif
