/**
    @file   potime.cpp
    @brief  Portable system-time routines.

    Copyright &copy; 1997-2003 by Josef Pelikan, MFF UK Prague
        http://cgg.ms.mff.cuni.cz/~pepca/
    @author PE
    @since  7.12.2001
    @date   12.2.2003
*/

#include "Es/essencepch.hpp"

//-----------------------------------------------------------
//  Actual system time:

#ifdef _WIN32

#include "Es/Common/win.h"

#if _MSC_VER<1300
  #include <largeint.h>
#endif
#include "Es/Common/global.hpp"

static LARGE_INTEGER hpcFrequency;

static bool isHpc = false;

void startSystemTime ()
    // check the high-performance counter
{
    if ( isHpc ) return;
    LARGE_INTEGER frequency;
    isHpc = (QueryPerformanceFrequency(&frequency) != 0);
    if ( isHpc ) hpcFrequency = frequency;
}

static class Init
{
public:
    Init()
    {
        startSystemTime();
    }
} SInit;

unsigned getClockFrequency ()
    // clock frequency in Hz
{
    return( isHpc ? (hpcFrequency.HighPart ? UINT_MAX : hpcFrequency.LowPart) : 100 );
}

unsigned64 getSystemTime ()
    // returns actual system time in micro-seconds
{
    if ( isHpc ) {
        LARGE_INTEGER count;
        if ( QueryPerformanceCounter(&count) ) {
            #if _MSC_VER>=1300
            LARGE_INTEGER sec;
            LARGE_INTEGER remainder;
            sec.QuadPart = count.QuadPart/hpcFrequency.QuadPart;
            remainder.QuadPart = count.QuadPart%hpcFrequency.QuadPart;
            #else
            LARGE_INTEGER remainder;
            LARGE_INTEGER sec = LargeIntegerDivide(count,hpcFrequency,&remainder);
            #endif
                // time = sec + (remainder/hpcFrequency)
            return( (unsigned64)sec.QuadPart * 1000000U +
                    ((unsigned64)remainder.QuadPart * 1000000U) / (unsigned64)hpcFrequency.QuadPart );
            }
        }
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return( ((((unsigned64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime + 5) / 10 );
}

#else

#include <sys/time.h>
#include "Es/Common/global.hpp"

void startSystemTime ()
{
}

unsigned getClockFrequency ()
    // clock frequency in Hz
{
    return 1000000000; // ProfileTime on linux is implemented to return nanoseconds precision
}

#ifdef HAS_CLOCK_GETTIME

    // "clock_gettime()" is defined:

unsigned64 getSystemTime ()
    // returns actual system time in micro-seconds
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    return( ts.tv_sec * 1000000L + (ts.tv_nsec + 500L) / 1000L );
}

#elif defined(HAS_GETTIMEOFDAY)

    // "gettimeofday()" is defined:

unsigned64 getSystemTime ()
    // returns actual system time in micro-seconds
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return( (unsigned64)tv.tv_sec * 1000000LL + tv.tv_usec );
}

#else

    // neither "clock_gettime()" nor "gettimeofday()" defined:

#  include <sys/timeb.h>

unsigned64 getSystemTime ()
    // returns actual system time in micro-seconds
{
    struct timeb tb;
    ftime(&tb);
    return( 1000 * (tb.millitm + 1000 * (unsigned64)tb.time) );
}

#endif

/*!
\patch 5164 Date 9/18/2007 by Bebul
- Fixed: spinning in select() on Linux Dedicated Server
*/
#include <sched.h>
void sleepMs ( unsigned ms )
{
  if (ms)
  {
    struct timeval timeout =
    { ms/1000L, (ms%1000L)*1000L };             // seconds, micro-seconds
    select(0,NULL,NULL,NULL,&timeout);
  }
  else
  { // zero time in Win32 means the caller wants the thread to relinquish the remainder of       // its time slice to any other thread of equal priority that is ready to run
    // On Linux it can be done by calling sched_yield() - relinquish the CPU (POSIX)
    sched_yield(); 
  }
}

#endif
