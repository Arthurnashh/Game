#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DEBUG_TRAP_HPP
#define _DEBUG_TRAP_HPP

#include <Es/Types/scopeLock.hpp>

class DebugThreadWatch;

struct PauseChecking;

/// debugger - making sure process is not frozen
class Debugger
{
	bool _isDebugger;
  bool _enableThreadWatch;

	SRef<DebugThreadWatch> _watch;

public:
	Debugger();
	~Debugger();

	bool IsDebugger() const {return _isDebugger;}
	void ForceLogging();
	void ProcessAlive();
	void NextAliveExpected( int timeout );

	bool CheckingAlivePaused();

	void PauseCheckingAlive();
	void ResumeCheckingAlive();

  struct PauseCheckingTraits
  {
    static void Lock(Debugger &item) {item.PauseCheckingAlive();}
    static void Unlock(Debugger &item) {item.ResumeCheckingAlive();}
  };
  typedef ScopeLock<Debugger,PauseCheckingTraits> PauseCheckingScope;
  
protected:
	void StartWatchThread();
};

#define BREAK() {static bool disableBreak;if (!disableBreak) {FailHook("BREAK");}}

extern Debugger GDebugger;

#endif
