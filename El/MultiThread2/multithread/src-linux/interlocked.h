/*
GNU C Interlocked functions for x86
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

#ifndef _MULTITHREAD_LINUX_INTERLOCKED_H_
#define _MULTITHREAD_LINUX_INTERLOCKED_H_

namespace MultiThread
{


static inline long XAdd(long volatile *mem, long offset)
{
    asm volatile
        (
        "lock xadd %1, %0"
        : "=m" (*mem), "=q" (offset)
        : "m" (*mem), "1" (offset)
        );
    return offset;    
}



static inline long  MTIncrement(long volatile *mem) 
{
    return XAdd(mem,1)+1;   
}

static inline long  MTDecrement(long volatile *mem) 
{
    return XAdd(mem,-1)-1;   
}

static inline long MTExchangeAdd(long volatile *mem, long value)
{
    return XAdd(mem,value);
} 

static inline long MTExchange(long volatile *mem, long iParam)
{
    asm volatile
        (
        "lock xchg %1, %0"
        : "=m" (*mem), "=q" (iParam)
        : "m" (*mem), "1" (iParam)
        :"cc"
        );
    return iParam;     
}

static inline long MTCompareExchange(long volatile *mem, long iParam, long iOldValue)
    {    
    asm volatile
        (
        "lock cmpxchg %1, %0"
        : "=m" (*mem), "=q" (iParam), "=a" (iOldValue)
        : "m" (*mem), "1" (iParam), "a" (iOldValue)
        :"cc"
        );
    return iOldValue;
    }
}


#endif /*INTERLOCKED_H_*/

