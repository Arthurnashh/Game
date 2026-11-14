#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   potime.hpp
    @brief  Portable system-time routines.

    Copyright &copy; 1997-2002 by Josef Pelikan, MFF UK Prague
        http://cgg.ms.mff.cuni.cz/~pepca/
    @author PE
    @since  7.12.2001
    @date   10.6.2002
*/

#ifndef _POTIME_H
#define _POTIME_H

//-----------------------------------------------------------
//  Actual system time:

/**
    Initialize the system timer (for high-performance timers on Win32).
*/
extern void startSystemTime ();

/**
    Get system clock (high-performacne real-time clock) frequency in Hz.
*/
extern unsigned getClockFrequency ();

/**
    Actual system time.
    @return Actual system time in micro-seconds.
*/
extern unsigned64 getSystemTime ();

//-----------------------------------------------------------
//  Sleep API:

#ifdef _WIN32

/// Sleep in milliseconds
#  define SLEEP_MS(t) Sleep(t)

#else

/// Sleep in milliseconds
#  define SLEEP_MS(t) sleepMs(t)

extern void sleepMs ( unsigned ms );

#endif

#endif
