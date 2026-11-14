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
#include "iblocker.h"
#include "CriticalSection.h"


namespace MultiThread
{

  ///Class provides simple wrapper for critical section.
  /**
  Using this blocker, you will able use it as both IBlocker an CriticalSection
  */
  class CriticalSectionBlocker : public IBlocker
  {
    CriticalSection _critSect;
  public:

    ///Constructs critical section blocker
    /**
    @param spinCount - specifies count of spins before Critical Section sleeps the thread, 
    if section is locked.
    */
    CriticalSectionBlocker(unsigned long spinCount=0):_critSect(spinCount) {}

    bool Acquire(unsigned long timeout=-1)
    {
      return _critSect.Lock(timeout);
    }

    void Release()
    {
      return _critSect.Unlock();
    }

    void Block() {}

    CriticalSection *operator->() {return &_critSect;}
    const CriticalSection *operator->() const {return &_critSect;}  

    operator CriticalSection &() {return _critSect;}
    operator const CriticalSection &() const {return _critSect;}  
  };

};