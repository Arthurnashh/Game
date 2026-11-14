/*
Posix thread object 
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
#ifndef _MULTITHREAD_INTERNAL_LINUX_POSIXTHREAD_H_
#define _MULTITHREAD_INTERNAL_LINUX_POSIXTHREAD_H_

#include "Blocker.h"
#include "../SyncRefCount.h"
#include "PosixEvent.h"  

namespace MultiThread
{

    class PosixThread:  public BlockerPlug
    {
        PosixEvent _finishWait;
        unsigned int _exitCode;
    public:            
    	PosixThread();
        ~PosixThread();      
        
        static Ref<BlockerPlug> GetActiveThreadPlug();
        
        unsigned int GetExitCode() const {return _exitCode;}
        
        void FinishNotify(unsigned int exitCode);
        
        bool WaitForThreadEnd(unsigned long timeout=(unsigned long )-1);
        
        BlockerSlot *GetEvent() {return &_finishWait;}

        // MT Safe operators
        void* operator new ( size_t size );
        void* operator new ( size_t size, const char *file, int line );
        void  operator delete ( void *mem );
    };

}

#endif /*POSIXTHREAD_H_*/
