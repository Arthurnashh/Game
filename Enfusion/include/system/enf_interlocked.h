//-----------------------------------------------------------------------------
// File: enf_interlocked.h
//
// Desc: Atomic operations
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_INTERLOCKED_H
#define ENF_INTERLOCKED_H

#include "enf_config.h"
#include "common/enf_inttypes.h"
#ifdef WIN32
	#ifndef WINDOWS_LEAN_AND_MEAN
		#define WINDOWS_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#elif defined(ENF_PS3)
	#include <cell/atomic.h>
#elif defined(__APPLE__)
	#include <libkern/OSAtomic.h>
#endif

namespace enf
{
/** @addtogroup System
@{*/

#if defined(WIN32)
	#define IL_EXCHANGE(target, value) InterlockedExchange(&target, value)
	#define IL_EXCHANGE_ADD(target, value) InterlockedExchangeAdd(&target, value)
	#define IL_COMPARE_EXCHANGE(target, exch, comp) InterlockedCompareExchange(&target, exch, comp)
	#define IL_READ(target) InterlockedCompareExchange(&target, 0, 0)
	#define IL_INCREMENT(target) InterlockedIncrement(&target)
	#define IL_DECREMENT(target) InterlockedDecrement(&target)
	extern "C"
	{
		void _ReadWriteBarrier();
		#pragma intrinsic(_ReadWriteBarrier)
	}
#elif defined(ENF_XENON)
	#define IL_EXCHANGE(target, value) InterlockedExchange(&target, value)
	#define IL_EXCHANGE_ADD(target, value) InterlockedExchangeAdd(&target, value)
	#define IL_COMPARE_EXCHANGE(target, exch, comp) InterlockedCompareExchange(&target, exch, comp)
	#define IL_READ(target) InterlockedCompareExchange(&target, 0, 0)
	#define IL_INCREMENT(target) InterlockedIncrement(&target)
	#define IL_DECREMENT(target) InterlockedDecrement(&target)
	extern "C"
	{
		void _ReadWriteBarrier();
		#pragma intrinsic(_ReadWriteBarrier)
	}
#elif defined(ENF_PS3)
	#define IL_EXCHANGE(target, value) cellAtomicStore32((uint32_t*)&target, (uint32_t)value)
	#define IL_EXCHANGE_ADD(target, value) cellAtomicAdd32((uint32_t*)&target, (uint32_t)value)
	#define IL_COMPARE_EXCHANGE(target, exch, comp) cellAtomicCompareAndSwap32((uint32_t*)&target, (uint32_t)comp, (uint32_t)exch)
	#define IL_READ(target) cellAtomicCompareAndSwap32((uint32_t*)&target, 0, 0)
	#define IL_INCREMENT(target) (cellAtomicIncr32((uint32_t*)&target) + 1)
	#define IL_DECREMENT(target) (cellAtomicDecr32((uint32_t*)&target) - 1)
	#define _ReadWriteBarrier() __lwsync()
#elif defined(__APPLE__)
	#define IL_EXCHANGE(target, value)  (target = value)
	#define IL_EXCHANGE_ADD(target, value) OSAtomicAdd32(value, &target)
	ENF_INLINE IL_INT32 _IL_COMPARE_EXCHANGE(IL_INT32& target, int exch, int comp)
	{
		int ret = target;
		if(OSAtomicCompareAndSwap32(comp, exch, &target))
			return comp;
		else
			return ret;
	}
	#define IL_COMPARE_EXCHANGE(target, exch, comp) enf::_IL_COMPARE_EXCHANGE(target, exch, comp)
	#define IL_READ(target) (target)
	#define IL_INCREMENT(target) OSAtomicIncrement32(&target)
	#define IL_DECREMENT(target) OSAtomicDecrement32(&target)	
	#define _ReadWriteBarrier() OSMemoryBarrier();
#endif

//@}

}

#endif //ENF_INTERLOCKED_H
