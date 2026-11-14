/*
Simulation of semaphore in posix threads
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
#ifndef _MULTITHREAD_INTERNAL_LINUX_POSIXSEMAPHORE_H_
#define _MULTITHREAD_INTERNAL_LINUX_POSIXSEMAPHORE_H_

#include "Blocker.h"
#include <map> 

namespace MultiThread
{
#ifdef BREDY_OLD_IMPLEMENTATION
    class PosixSemaphore : public BlockerSlot
    {
        Ref<BlockerPlug> *_objects;
        int *_counters;
        int _limit;
        
        int FindObjectOrEmpty(BlockerPlug *object);
    public:
    	PosixSemaphore(unsigned int limit);
    	virtual ~PosixSemaphore();
        
        bool Acquire(BlockerPlug *plug, bool nowait);
        void Release(BlockerPlug *plug);    
        
    };
#else
    class PosixSemaphore : public BlockerSlot
    {
      int _counter;
      int _limit;

    public:
      PosixSemaphore(unsigned int limit, unsigned int initialCount);
      virtual ~PosixSemaphore();

      bool Acquire(BlockerPlug *plug, bool nowait);
      void Release(BlockerPlug *plug=NULL); //parameter is not used, but it is called using IS() interface

      // MT Safe operators
      void* operator new ( size_t size );
      void* operator new ( size_t size, const char *file, int line );
      void  operator delete ( void *mem );
    };
#endif
}

#endif /*POSIXSEMAPHORE_H_*/
