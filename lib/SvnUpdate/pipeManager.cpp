#include "Es/Common/win.h"
#include "pipeManager.h"
#include "Es/strings/rString.hpp"

//initialize static
const char *PipeManager::CmdClosing="closing";
const char *PipeManager::CmdFailed="failed";

bool PipeManager::Update()
{
  DWORD bytesAvail;
  BOOL retVal = PeekNamedPipe(_hIn, NULL, 0, NULL, &bytesAvail, NULL);
  if (!retVal)
  { // DayZUpdater has possibly finished (upgrade...)
    if ( GetLastError()==ERROR_BROKEN_PIPE ) 
    { // The pipe has been ended.
      _hIn = INVALID_HANDLE_VALUE;
      return false;
    }
  }
  else if (bytesAvail>0)
  {
    char buf[1024];
    DWORD bytes=0;
    if (!ReadFile(_hIn, buf, 1023, &bytes, NULL))
    {
      return false;
    }
    else
    {
      _buf = _buf + std::string(buf, bytes);
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
          _commands.push(std::string(data, len));
        }
      } while (ptr);
      _buf = data ? data : std::string();
    }
  }

  return true;
}

// get the command from queue if any
std::string PipeManager::Receive()
{
  if ( !_commands.empty() )
  {
    std::string retval = _commands.front();
    _commands.pop();
    return retval;
  }
  return std::string();
}

//send the command
void PipeManager::Send(std::string command)
{
  if ( Ready() && command.length() )
  {
    std::string cmd = std::string("<") + command + std::string(">");
    _write(_fdOut, cmd.c_str(), cmd.length());
  }
}
