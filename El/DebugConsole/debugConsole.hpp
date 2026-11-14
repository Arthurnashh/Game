#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_DEBUG_CONSOLE_HPP
#define _EL_DEBUG_CONSOLE_HPP

class IDebugCommandCommandHandler
{
  public:
  virtual void Do(const char *command) = 0;
};


class IDebugConsole
{
  public:
  /// perform regular command handling
  virtual void HandleCommands() {}
  /// output any string to the console
  virtual void Printf(const char *format, ...) {}

  /// anyone who wants to parse command can register here
  virtual void Register(IDebugCommandCommandHandler *handler){}
};

extern IDebugConsole *GDebugConsole;

#endif
