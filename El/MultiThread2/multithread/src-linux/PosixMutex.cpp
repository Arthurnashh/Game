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
#include "PosixMutex.h"

namespace MultiThread
{

    PosixMutex::PosixMutex(BlockerPlug *owner):_owner(owner),_recursionCount(0)
    {
        
    }

    bool PosixMutex::Acquire(BlockerPlug *plug, bool nowait)
    {        
        bool res=true;
        //lock this part
        Lock();
        //no owner?
        if (_owner==0)
        {
            //set plug as owner
            _owner=plug;
            //setup recursion count
            _recursionCount=1;
        }
        //already owner?
        else if (plug==_owner) 
        {
            //increase recursion count
            _recursionCount++;
        }
        //another owner?
        else
        {
            //signup to queue
            //return result
            BlockerSlot::Subscribe(plug,nowait);
            res=false;
        }
        //unlock this part
        Unlock();
        return res;
    }
    
    void PosixMutex::Release(BlockerPlug *plug)
    {
        //lock this part
        Lock();
        //release is allowed only when plug is owner
        AssertDesc(_owner==plug,"Release of not owned mutex");
        //
        if (_owner==plug)
        {
            //decerase recursion count
            _recursionCount--;
            //we reached zero?
            if (_recursionCount==0)
            {
                //release next waiting plug
                Ref<BlockerPlug> pp=ReleaseOne(true,true);
                //new owner (pp==NULL, no owner)
                _owner=pp;
                //setup recursion count
                _recursionCount=1;
            }
        }
        //unlock
        Unlock();
    }    


}

#include "Es/Memory/checkMem.hpp"
#include "Es/Memory/normalNew.hpp"

namespace MultiThread
{
  void* PosixMutex::operator new ( size_t size )
  {
    return safeNew(size);
  }

  void* PosixMutex::operator new ( size_t size, const char *file, int line )
  {
    return safeNew(size);
  }

  void PosixMutex::operator delete ( void *mem )
  {
    safeDelete(mem);
  }
}
