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
#include "PosixThread.h"
#include "PosixEvent.h"

namespace MultiThread
{

    PosixEvent::PosixEvent(bool signaled, bool pauto):_state(signaled),_auto(pauto)
    {
    }

    bool PosixEvent::Acquire(BlockerPlug *plug, bool nowait)
    {
        Lock();
        if (_state)
        {
            if (_auto) _state=false;
            Unlock();
            return true;
        }
        Subscribe(plug,nowait);                
        Unlock();
        return false;
    }

    void PosixEvent::Block()
    {
        Lock();
        _state=false;
        Unlock();
    }
    
    void PosixEvent::Unblock()
    {
        Lock();
        if (_auto)
        {
            Ref<BlockerPlug> pp=ReleaseOne(true,true);
            if (pp==0) _state=true;                      
        }
        else
        {            
            ReleaseAll(true,true);
            _state=true;
        }
        Unlock();
    }    
    
    void PosixEvent::Pulse()
    {
        if (_auto)
        {
            ReleaseOne();
        }
        else
        {
            ReleaseAll();
        }
    }

}

#include "Es/Memory/checkMem.hpp"
#include "Es/Memory/normalNew.hpp"

namespace MultiThread
{
  void* PosixEvent::operator new ( size_t size )
  {
    return safeNew(size);
  }

  void* PosixEvent::operator new ( size_t size, const char *file, int line )
  {
    return safeNew(size);
  }

  void PosixEvent::operator delete ( void *mem )
  {
    safeDelete(mem);
  }
}
