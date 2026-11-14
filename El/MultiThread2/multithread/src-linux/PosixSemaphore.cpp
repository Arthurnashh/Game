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


#include "common_linux.h"
#include "PosixSemaphore.h"

namespace MultiThread
{
#ifdef BREDY_OLD_IMPLEMENTATION    
    PosixSemaphore::PosixSemaphore(unsigned int limit):_objects(0),_counters(0),_limit(limit)
    {    
      try
      {
        _objects=new Ref<BlockerPlug>[_limit];
        _counters=new int[_limit];
      }
      catch(...)
      {
        delete [] _objects;
        delete [] _counters;
        throw;
      }
      for (int i=0;i<_limit;i++) {_objects[i]=0;_counters[i]=0;}
    }
    
    int PosixSemaphore::FindObjectOrEmpty(BlockerPlug *object)
    {
        int i=0,e=-1;
        while (i<_limit && _objects[i]!=object) 
        {
            if (_objects[i]==0) e=i;
            i++;
        }
        if (i==_limit) return e;
        return i; 
    }
    
    
    PosixSemaphore::~PosixSemaphore()
    {
        delete [] _objects;
        delete [] _counters;
    }

    bool PosixSemaphore::Acquire(BlockerPlug *plug, bool nowait)
    {
        Assert(plug!=0);
        bool res;
        Lock();
        int p=FindObjectOrEmpty(plug);
        if (p==-1) //not object nor empty
        {
            BlockerSlot::Subscribe(plug,nowait);
            res=false;
        }
        else   
        {
            res=true;
            if (_objects[p]==plug) _counters[p]++;
            else
            {
                Assert(_objects[p]==0);
                _objects[p]=plug;
                _counters[p]=1;
            }                      
        }
        Unlock();
        return res;
    }
    
    void PosixSemaphore::Release(BlockerPlug *plug)
    {
        Assert(plug!=0);        
        Lock();
        int p=FindObjectOrEmpty(plug);
        AssertDesc(p!=-1,"Not owner of the semaphore");
        if (p!=-1)
        {
            if (_objects[p]==plug)
            {
                if (--_counters[p]==0)
                {
                    _objects[p]=ReleaseOne(true,true);
                    _counters[p]=1;
                }
            }
        }
        Unlock();
    }    
#else    
  PosixSemaphore::PosixSemaphore(unsigned int limit, unsigned int initialCount)
    :_counter(initialCount),_limit(limit)
  {
  }


  PosixSemaphore::~PosixSemaphore()
  {
  }

  bool PosixSemaphore::Acquire(BlockerPlug *plug, bool nowait)
  {
    Assert(plug!=0);
    bool res;
    Lock();
    if (_counter>0)
    {
      _counter--;
      res = true;
    }
    else
    {
      BlockerSlot::Subscribe(plug,nowait);
      res = false;
    }
    Unlock();
    return res;
  }

  void PosixSemaphore::Release(BlockerPlug *plug)
  {
    Lock();
    Ref<BlockerPlug> pp=ReleaseOne(true,true);
    if (pp==NULL)
    {
      //there was no plug waiting, so increase the counter
      _counter++;
    }
    //else there was some plug waiting, so counter will be not affected and there is no work left
    Unlock();
  }    
#endif

}

#ifndef BREDY_OLD_IMPLEMENTATION    
#include "Es/Memory/checkMem.hpp"
#include "Es/Memory/normalNew.hpp"

namespace MultiThread
{
  void* PosixSemaphore::operator new ( size_t size )
  {
    return safeNew(size);
  }

  void* PosixSemaphore::operator new ( size_t size, const char *file, int line )
  {
    return safeNew(size);
  }

  void PosixSemaphore::operator delete ( void *mem )
  {
    safeDelete(mem);
  }
}
#endif
