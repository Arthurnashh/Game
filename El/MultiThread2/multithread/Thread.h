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
#include "ThreadBase.h"

namespace MultiThread
{

  ///Class provides wrapper for any IRunnable class type
  class Thread : public ThreadBase
  {
    IRunnable *_runnable;
  public:

    ///Creates thread object and wraps runnable class
    Thread(IRunnable *r):_runnable(r)  {}
    ///Creates thread object and wraps runnable class
    Thread(IRunnable *r,Priority priority, size_t initStack=0):_runnable(r),ThreadBase(priority,initStack) {}


    ///Implements Run - starts runnable
    unsigned long Run()
    {
      return _runnable->Run();
    }

    ///Returns pointer to runnable class
    virtual IRunnable *GetRunnable() const {return _runnable;}

  };


  ///Implements dynamic allocated Thread that starts statically or auto allocated runnable
  /**
  When thread exists, Thread is destroyed, bud runnable leaved.
  */
  class ThreadDyn: public Thread
  {
  public:
    ThreadDyn(IRunnable *r): Thread(r) {}
    ThreadDyn(IRunnable *r,Priority priority, size_t initStack=0):Thread(r,priority,initStack) {}
    virtual void Done(bool initFailed)
    {
      delete this;
    }
  };

};

