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

namespace MultiThread
{

    Ref<BlockerPlug> PosixThread::GetActiveThreadPlug()
    {
        ThreadBase_InfoStruct *curThread=ThreadBase_InfoStruct::GetCurrentThread();
        //test for active thread
        AssertDesc(curThread,"This thread is not controlled by MultiThread library. You cannot call wait function, until Thread instance is associated with the thread");
        //no active thread is error
        if (curThread==0) return 0;
        //get active thread's plug
        return static_cast<BlockerPlug *>(curThread->_threadInfo.GetRef());        
        
    }
    
    PosixThread::PosixThread():_finishWait(false,false),_exitCode((unsigned int)-1)
    {
        DebugPrint1("... Thread Object created, %08X\n",this)
    }
    PosixThread::~PosixThread()
    {
        DebugPrint1("... Thread Object finally destroyed, %08X\n",this)
    }  
    
    void PosixThread::FinishNotify(unsigned int exitCode)
    {
        _exitCode=exitCode;
        _finishWait.Unblock();
    }
    
    bool PosixThread::WaitForThreadEnd(unsigned long timeout)
    {
        return ThreadInternal::WaitForSingleObject(&_finishWait,timeout);
    }
}

#include "Es/Memory/checkMem.hpp"
#include "Es/Memory/normalNew.hpp"

namespace MultiThread
{
  void* PosixThread::operator new ( size_t size )
  {
    return safeNew(size);
  }

  void* PosixThread::operator new ( size_t size, const char *file, int line )
  {
    return safeNew(size);
  }

  void PosixThread::operator delete ( void *mem )
  {
    safeDelete(mem);
  }
}
