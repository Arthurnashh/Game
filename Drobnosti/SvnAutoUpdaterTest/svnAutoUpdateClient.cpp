#include <Es/essencepch.hpp>
#include <El/elementpch.hpp>

#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <Es/Strings/rString.hpp>

#include "svnAutoUpdateClient.h"

//@{ PipeManager initialization and commands
const string CmdThx("thanks for ");
const string CmdCommandline("commandline: ");
const string CmdUpdateDir("updateDir: ");
const string CmdSvnUrl("url: ");
const string CmdShutDown("shutDown");
const string CmdCurDir("workingDir: ");  
const string CmdCopyDir("copyDir: ");  
const string CmdUpToDate("up-to-date");
const string CmdCheckout("checkouting");
const string CmdOutdated("updating");
//@}

bool fexists(const char *filename)
{
  std::ifstream ifile(filename);
  return ifile!=0;
}

bool SVNAutoUpdaterClient::Init(const char *svnAUPath, const char *svnURL, const char *user, const char *passwd)
{
  if ( fexists(svnAUPath) )
  {
    // create in/out pipes 
    _pipe(&_pipes[0], 4096, O_TEXT); // I read/she writes pipe
    _pipe(&_pipes[2], 4096, O_TEXT); // She reads/I write pipe

    RString parIn = Format("/in=%d", _pipes[2]);
    RString parOut= Format("/out=%d",_pipes[1]);
    RString parUser = Format("/login=%s", user);
    RString parPasswd= Format("/password=%s", passwd);
    hProcess = (HANDLE)spawnl(P_NOWAIT, svnAUPath, svnAUPath, 
                              cc_cast(parIn), 
                              cc_cast(parOut), 
                              cc_cast(parUser),
                              cc_cast(parPasswd),
                              NULL);

    // close her pipes and restore stdin/stdout
    close(_pipes[2]);
    close(_pipes[1]);

    if(hProcess && (unsigned int)hProcess!=0xffffffff)
    {
      _pipeManager.Init(PipeIn(), PipeOut());
      if ( _pipeManager.Ready() )
      {
        // send command line
        LPSTR cmdLine = GetCommandLine();
        _pipeManager.Send(CmdCommandline + cmdLine);

        // send working directory
        {
          char cwdBuf[1024];
          GetCurrentDirectory(1024, cwdBuf);
          _pipeManager.Send(CmdCurDir + cwdBuf);
        }

        // send copy directory
        {
          char copyBuf[1024];
          GetCurrentDirectory(1024, copyBuf);
          _pipeManager.Send(CmdCopyDir + copyBuf);
        }

        // send svn url to check new version for
        _pipeManager.Send(CmdSvnUrl + string(svnURL));

        // send update dir (working directory which is or will be checkouted to SvnUrl)
        _pipeManager.Send(CmdUpdateDir + string("O:\\tmp\\bebulek"));
  
        _ready = true;
      }
    }
  }
  return _ready;
}

bool SVNAutoUpdaterClient::Update()
{
  _pipeManager.Update();

  // process all commands
  for (string command=_pipeManager.Receive(); !command.empty(); command=_pipeManager.Receive())
  {
    if ( !strnicmp(command.c_str(), CmdThx.c_str(), CmdThx.length()) )
    {
      printf("<%s>\n", command.c_str());
    }
    else if ( !stricmp(command.c_str(), CmdShutDown.c_str()) )
    {
      printf("<%s>\n", command.c_str());
      return false; //end application
    }
    else if ( !stricmp(command.c_str(), PipeManager::CmdClosing) )
    {
      printf("The SVNAutoUpdater has finished!\n");
    }
    else if ( !stricmp(command.c_str(), CmdUpToDate.c_str()) )
    {
      printf("The current version is up-to-date!\n");
#if 0
      _pipeManager.Send(CmdShutDown);
#endif
    }
    else if ( !stricmp(command.c_str(), CmdOutdated.c_str()) )
    {
      printf("The current version is outdated! Svn update in progress...\n");
    }
    else if ( !stricmp(command.c_str(), CmdCheckout.c_str()) )
    {
      printf("The aplication is not under svn control yet, checkout in progress...\n");
    }
    else if ( !stricmp(command.c_str(), PipeManager::CmdFailed) )
    {
      printf("The autoUpdate functionality is not available!\n");
    }
    else printf("ERROR UNKNOWN: %s\n", command.c_str());
  }

  return true;
}
