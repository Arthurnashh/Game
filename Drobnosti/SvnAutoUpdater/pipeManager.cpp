#include "pipeManager.h"

//initialize static
const char *PipeManager::CmdClosing="closing";
const char *PipeManager::CmdFailed="failed";

PipeManager::~PipeManager()
{
  //Send(PipeManager::CmdClosing);
}

void PipeManager::Update()
{
  DWORD bytesAvail;
  BOOL retVal = PeekNamedPipe(_hIn, NULL, 0, NULL, &bytesAvail, NULL);
  if (retVal && bytesAvail>0)
  {
    char buf[1024];
    DWORD bytes=0;
    if (!ReadFile(_hIn, buf, 1023, &bytes, NULL))
    {
      return;
    }
    else
    {
      _buf = _buf + string(buf, bytes);
      const char *data = _buf.c_str();
      const char *ptr = data;
      do
      {
        // move data pointer to the start of next command
        data = strchr(ptr, '<');
        if (!data) break;
        ptr = strchr(data, '>');
        if (!ptr) break;
        data++; 
        int len = ptr-data;
        if (len>0)
        {
          _commands.push(string(data, len));
        }
      } while (ptr);
      _buf = data ? data : string();
    }
  }
}

// get the command from queue if any
string PipeManager::Receive()
{
  if ( !_commands.empty() )
  {
    string retval = _commands.front();
    _commands.pop();
    return retval;
  }
  return string();
}

//send the command
void PipeManager::Send(const string command)
{
  if ( Ready() && command.length() )
  {
    string cmd = string("<") + command + string(">");
    _write(_fdOut, cmd.c_str(), cmd.length());
  }
}
