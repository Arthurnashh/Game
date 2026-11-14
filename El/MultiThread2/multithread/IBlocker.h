/*
Copyright (C)2006 bo Ondrej Novak
http://bredy.jinak.cz
novak.on@centrum.cz


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once

namespace MultiThread
{


///Defines standart blocker
/**
Blockers are used for block thread from continuing. If you want
to synchronize one with another, blocker is best choice. 
Depend of blocking type, choose one of derived classes.

EventBlocker<br>
ThreadBlocker<br>
ProcessBlocker<br>
CriticalSectionBlocker<br>
MutexBlocker<br>
SemaphoreBlocker<br>
BariereBlocker<br>
TimerBlocker<br>
SleepBlocker<br>
WinMessageBlocker<br>


*/
class IBlocker
{
public:
  ///Call this function to acquire the blocker
  /** 
  @param timeout defines timeout for waiting
  @return true, if thread was unblocked or false, when timeout period elapses
  */
  virtual bool Acquire(unsigned long timeout=-1)=0;  

  ///Call this function to release the blocker
  /**
  Each acquired blocker should be release. But some types of blocker doesn't handle 
  this operation
  */
  virtual void Release()=0;  

  ///Synonymum for Acquire- especially where blocker is used as lock
  bool Lock(unsigned long timeout) {return Acquire(timeout);}
  ///Synonymum for Release - especially where blocker is used as lock
  void Unlock() {Release();}
  
  virtual ~IBlocker() {}
};

};
