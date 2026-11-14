/*
Simulation of windows mutex in posix threads
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

#ifndef _MULTITHREAD_INTERNAL_LINUX_POSIXMUTEX_H_
#define _MULTITHREAD_INTERNAL_LINUX_POSIXMUTEX_H_

#include "Blocker.h"

namespace MultiThread
{

class PosixMutex : public BlockerSlot
{
    Ref<BlockerPlug> _owner;
    int _recursionCount;
public:
	PosixMutex(BlockerPlug *owner=0);

    bool Acquire(BlockerPlug *plug, bool nowait);
    void Release(BlockerPlug *plug);

    // MT Safe operators
    void* operator new ( size_t size );
    void* operator new ( size_t size, const char *file, int line );
    void  operator delete ( void *mem );
};

}

#endif /*POSIXMUTEX_H_*/
