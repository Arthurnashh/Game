/// Minimal implementation to use imexhnd.cpp without debugTrap
#include <El/elementpch.hpp>
#include <Es/Common/win.h>
#include <Es/Files/commandLine.hpp>
#include "debugTrap.hpp"


class DebugThreadWatch
{
};

Debugger GDebugger;


Debugger::Debugger()
{
  _isDebugger=false;
  _enableThreadWatch=false;
}

Debugger::~Debugger()
{
}

void Debugger::ForceLogging(){}
void Debugger::ProcessAlive(){}
void Debugger::NextAliveExpected( int timeout ){}

bool Debugger::CheckingAlivePaused(){return false;}

void Debugger::PauseCheckingAlive(){}
void Debugger::ResumeCheckingAlive(){}


void Debugger::StartWatchThread() {}

