#include <string.h>
#include <stdio.h>
#include <windows.h>

void TrimCRLF(char *str)
{
  int l = strlen(str);
  while (l>0 && (str[l-1]=='\n' || str[l-1]=='\n')) str[l-1]=0;
}

struct EchoThreadContext
{
  HANDLE hPipe;
  HANDLE hProcess;
};

DWORD CALLBACK EchoThread(void *context)
{
  EchoThreadContext *ctx = (EchoThreadContext *)context;
  // read the pipe, print any messages received
  for(;;) // The thread will be terminated if needed
  {
    if (WaitForSingleObject(ctx->hProcess,0)==WAIT_OBJECT_0)
    {
      ExitProcess(0);  // example only, do not do this in real application
    }
    // note: the pipe was created with PIPE_NOWAIT in the game, but this does not apply for this end
    const size_t bufSize = 4*1024;
    char buffer[bufSize+2];
    DWORD bytesLeft;
    if (!PeekNamedPipe(ctx->hPipe,NULL,0,NULL,NULL,&bytesLeft) || bytesLeft==0)
    {
      Sleep(10);
    }
    else
    {
      DWORD read = 0;
      BOOL ok = ReadFile(ctx->hPipe,buffer,sizeof(buffer),&read,NULL); 
      if (ok)
      {
        buffer[read] = '\n';
        buffer[read+1] = 0;
        fputs(buffer,stdout);
      }
    }
  }
  return 0;
}

int main(int argc, const char *argv[])
{
  // launch the game
  if (argc<3)
  {
    fprintf(stderr,"Usage: Launcher currentDirectory applicationFileName applicationArguments\n");
    return -1; // game exe name expected
  }
  const char *curDir = argv[1];
  const char *gameExe = argv[2];
  STARTUPINFO startupInfo;
  memset(&startupInfo, 0, sizeof(startupInfo));
  startupInfo.cb = sizeof(startupInfo); 
  PROCESS_INFORMATION pi;
  char commandLine[2048];
  #define PIPE_GUID "0cee18c8-b89a-4116-9e9f-47c48875830e"
  _snprintf(commandLine,sizeof(commandLine)-1,"%s -command=" PIPE_GUID "ExampleA2OALauncher",gameExe);
  for (int i=3; i<argc; i++)
  { // note: example only, buffer overrun possible
    strcat(commandLine," " );
    strcat(commandLine,argv[i]);
  }
  commandLine[sizeof(commandLine)-1]=0;
  if (!CreateProcess(NULL,commandLine,NULL,NULL,FALSE,0,NULL,curDir,&startupInfo,&pi))
  {
    HRESULT hr = GetLastError();
    fprintf(stderr,"Cannot create process '%s', error %x\n",gameExe,hr);
    return -2;
  }

  char pipeName[]="\\\\.\\pipe\\" PIPE_GUID "ExampleA2OALauncher";
  HANDLE hPipe; 

  // Try to open a named pipe; wait for it, if necessary. 
  DWORD startWait = GetTickCount();
  for(;;)
  { 
    hPipe = CreateFile(pipeName, GENERIC_READ|GENERIC_WRITE, 0 /* no sharing */, NULL, OPEN_EXISTING, 0, NULL);

    if (hPipe != INVALID_HANDLE_VALUE) 
    {
      // The pipe connected; change to message-read mode. 
      DWORD dwMode = PIPE_READMODE_MESSAGE; 
      SetNamedPipeHandleState( hPipe, &dwMode, NULL, NULL);
      break; // Break once the pipe is open
    }
    if (WaitForSingleObject(pi.hProcess,0)==WAIT_OBJECT_0)
    {
      fprintf(stderr, "Could not connect to a pipe: game terminated"); 
      return -3;
    }

    // wait for a while, the pipe may be not ready yet
    HRESULT hr = GetLastError();
    if (hr != ERROR_PIPE_BUSY) 
    {
      if (GetTickCount()-startWait>120000)
      {
        fprintf(stderr, "Could not connect to a pipe: error %x",hr); 
        return -4;
      }
      // wait for a short while before retrying
      Sleep(1000);
    }
    else
    {
      // All pipe instances are busy, so wait for a while before retrying
      WaitNamedPipe(pipeName, 10000);
    } 
  } 

  EchoThreadContext ctx;
  ctx.hPipe = hPipe;
  ctx.hProcess = pi.hProcess;
  HANDLE echoThread = CreateThread(NULL,16*1024,EchoThread,&ctx,0,NULL);

  char buffer[1024];
  int ret = 0;
  while (fgets(buffer,sizeof(buffer),stdin)!=NULL)
  {
    TrimCRLF(buffer);
    if (WaitForSingleObject(pi.hProcess,0)==WAIT_OBJECT_0) break;
    DWORD written;
    BOOL ok = WriteFile(hPipe,buffer,strlen(buffer),&written,NULL);
    if (!ok)
    {
      fprintf(stderr, "Error while writing to pipe: error %x",GetLastError()); 
      ret = -10;
      break;
    }
    if (!stricmp(buffer,"shutdown"))
    {
      // shutdown sent, do not wait any more
      break;
    }
  }
  TerminateThread(echoThread,-1); // example only, do not do this in real application, use proper threading primitives
  CloseHandle(echoThread);
  CloseHandle(hPipe);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return ret;
}

