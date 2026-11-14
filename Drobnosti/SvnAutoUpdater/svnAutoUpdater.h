#ifndef _MAIN_H_INCLUDED
#define _MAIN_H_INCLUDED

#include <windows.h>
#include "controls.h"
#include "pipeManager.h"
#include "SystemTraySDK.h"
#include "svnAccess.hpp"

#define USER_WM_CONNECT 0x50BA

class WinException
{
public:
    WinException (char* msg)
    : _err (GetLastError()), _msg(msg)
    {}
    DWORD GetError() const { return _err; }
    char* GetMessage () const { return _msg; }
private:
    DWORD _err;
    char* _msg;
};

class Controller;

//! Running Client process communicating with this SvnAutoUpdater Server
class SvnUpdaterClientApp
{
  class AutoUpdateConfig
  {
    const char *ConfigFilename() const { return "autoUpdate.info"; }
  public:
    //! current client game folder revision
    int revision;
    //! when SvnAutoUpdater server is synchronizing the clients folder, it writes its PID into lockPID line
    int lockPID;
    //! first load processed
    bool loaded;

    AutoUpdateConfig() : revision(-1), lockPID(-1), loaded(false) {}
    //! Load the current state of autoUpdate.info
    void Load(string configDir);
    //! Save the current state of autoUpdate.info
    void Save(string configDir);
  };

  friend DWORD WINAPI ThreadSynchronizeGameFolders(LPVOID lpParameter);

  //! communication pipe to running client process. 
  //! Pipe is created by client and passed through USER_WM_CONNECT window message
  PipeManager _pipeManager;
  //! handle to running client process 
  HANDLE      _hProcess;
  DWORD       _pid;  //process id (nice to know for debug purposes, to identify the client)
  //! true when application wants to be restarted after update is over
  bool _restartAfterShutDown;

  //{@ parameters read from parent application using pipe
  string      _commandLine;  //commandline to use to restart parent application
  string      _workingDir;   //working dir to use to restart parent application
  string      _copyDir;      //directory to copy directory structure to
  //@}

  AutoUpdateConfig _autoUpdateInfo;

  Controller *_parent;

public:
  SvnUpdaterClientApp(HANDLE hIn, HANDLE hOut, Controller *ctrl) : _restartAfterShutDown(false), _askedToShutdown(false), _pid(-1)
  {
    _hProcess = INVALID_HANDLE_VALUE; //we'll get the ProcessID value through pipe
    _pipeManager.Init(hIn,hOut);
    _parent = ctrl;
  }

  ~SvnUpdaterClientApp()
  {
    _pipeManager.Close();
    if (_hProcess!=INVALID_HANDLE_VALUE) 
    {
      CloseHandle(_hProcess);
    }
  }

  //! communicates through pipeManger
  void Update();

  //! ready until _hProcess is shutDown
  bool Ready() { return _pipeManager.Ready(); }

  bool RestartAfterShutDown() { return _restartAfterShutDown; }
  //! start app again
  void Restart();

  //! check the autoUpdate.info content and decide, whether folder need sync based on currentRev
  bool VersionUpToDate(int currentRev);

  //! only for debug purposes
  bool IsRunning() { return _hProcess!=INVALID_HANDLE_VALUE; }

  string CopyDir() const { return _copyDir; }
  DWORD PID() const { return _pid; };

  void Send(string cmd) { _pipeManager.Send(cmd); }

public:
  //! client was asked to shutdown (her data are obsolete)
  bool _askedToShutdown;
};

class Controller
{
  friend BOOL CALLBACK DialogProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
  friend DWORD WINAPI ThreadSvnCheckoutProc(LPVOID lpParameter);
  friend DWORD WINAPI ThreadSvnUpdateProc(LPVOID lpParameter);
  friend DWORD WINAPI ThreadSynchronizeGameFolders(LPVOID lpParameter);

  enum Phase {
    PUpToDate,              //check for svn update from time to time
    PUpdating,              //svnUpdate/svnCheckout is in progress
    PCopying                //copying working copy over applications folders
  };

  Phase _phase;

private:
  //! connected clients list
  vector<SvnUpdaterClientApp *> _clients;

  static bool _csInitialized;
  CRITICAL_SECTION cs;
  //@{ status and logLine window items can be changed only from main thread
  string _statusWaiting; 
  vector<string> _logLinesWaiting;
  //@}
  CSystemTray TrayIcon;

  //@{ svn controll variables
  SvnAccess _svnAcc;
  //! contains return value from SvnAccess functions (false means the SVN is unavailable for some reason)
  bool      _svnAccOK;
  bool      _svnErrorShown;
  bool      _svnAccRestarted;  //true when SvnAcc was restarted with username and password
  //@}

  string      _updateDir;    //svn working directrory to check

private:
  //! Connect new client
  void Connect (HWND hwnd, HANDLE hIn, HANDLE hOut);
  void Command (HWND hwnd, int controlID, int command);
  bool Minimize(HWND hwnd);
  //! Set stored values of logLines and status from main thread
  void UpdateStatusAndLog();

  //! Called regularly on WM_TIMER message
  void Process();

  void Enter() 
  { 
    if (!_csInitialized) { InitializeCriticalSection(&cs); _csInitialized=true; }
    EnterCriticalSection(&cs); 
  }
  void Leave() { LeaveCriticalSection(&cs); }

  //! check whether there is new Svn revision of our working copy
  void CheckForUpdate();
  //! Manage Pipe in/out
  void UpdatePipeManager();
  //! communicate with connected clients
  void UpdateClients();
  //! restart clients with _restartAfterShutDown set sharing given folder
  void RestartClients(string folder);
  //! test whether pipe and parent process are well initialized
  bool Ready() const;

  //! finds any dirty client, which is already not running and its Game directory is to be updated. NULL for no such client available.
  SvnUpdaterClientApp *GetClientToUpdate();
  //! ask client with obsolete data to shutDown
  void AskClientsToShutDown();
  //! clear clients which are already not running and not waiting to restart
  void ClearInactiveClients();

public:
  Controller(HWND hwnd);
  ~Controller ();

  //! set the _statusWaiting to be set by main thread as status
  void SetStatus(string status);
  //! add into _logLinesWaiting to be set by main thread as log line
  void SetLogLine(string logLine);

  void SetPhase(enum Phase phase);

  int RepositoryRevision() const { return _svnAcc.RepositoryRevision();}

private:
  HWND        _hwnd;           //window handle
  HANDLE      _hSvnThread;     //handle to thread doing svn checkout or update
  HANDLE      _hCopyDirThread; //handle to thread copying directory structure from svn working copy to game folders

  Edit        _editLog;
  Button      _clear;
  Edit        _status;
  CheckBox    _minSysTrayCheck;

  bool        _firstSvnCheckDone;
  //! it tracks the IconTray minimise, maximise...
  bool        _minimized;
};

#endif // _MAIN_H_INCLUDED
