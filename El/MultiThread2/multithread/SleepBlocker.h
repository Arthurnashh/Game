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
#include "IBlocker.h"
#include "ThreadBase.h"

namespace MultiThread
{


  ///Sleep blocker provides simple sleep operation
  /**
  When thread acquires this blocker, it must sleep for specified miliseconds.
  No other event can interrupt this sleeping.
  */
  class SleepBlocker : public IBlocker
  {
    unsigned long _time; ///< time in milliseconds
  public:
    ///Constructs SleepBlocker
    /**
    @param miliseconds - time in miliseconds to sleep
    */
    SleepBlocker(unsigned long miliseconds):_time(miliseconds) {}

    ///Acquires the blocker
    /**
    Thread will go to sleep
    @param timeout - maximum time for sleeping. If specified timeout is 
    lower then sleep-time, function does nothing and return false
    @return true - sleep done, false - timeout elapses.
    */
    bool Acquire(unsigned long timeout=-1)
    {
      if (timeout<time) return false;
      ThreadBase::Sleep(_time);
      return true;
    }

    /**
    @note Nothing
    */
    void Release() {}
  };

};