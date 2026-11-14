/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#ifdef WIN32
#include <windows.h>
#elif defined(__linux)
#include <sys/time.h>
#endif

#include <deque>
#include "Physics/smTypes.h"
#include "Util/smPrinter.h"

/**
*	smDateTime contains several utility methods to make working with time and
*	date easy.
*/
class smDateTime
{
public:
	/** Returns high precision CPU time in seconds. */
	static double getCPUTime()
	{
#ifdef WIN32
		static bool firstTime = true ;
		static LONGLONG			ticksPerSecond ;

		if ( firstTime )
		{
			LARGE_INTEGER freq ;
			if ( QueryPerformanceFrequency(  &freq ) != 0 )
				ticksPerSecond	=	freq.QuadPart ;

			firstTime = false ;
		}

		LARGE_INTEGER	ticks ;
		QueryPerformanceCounter ( &ticks );
		// Divide by frequency to get the time in seconds
		return (double) ticks.QuadPart / (double) ticksPerSecond ;
#else
        //
        //	Declare them static to improve performance of this
        //	method
        //
        static struct timeval tv ;
        static struct timezone tzDummy ;

        gettimeofday( &tv, &tzDummy );

        smTime::DoublePrec timeSec	= tv.tv_sec ;
        smTime::DoublePrec timeMSec = tv.tv_usec / 1000000.0f ;
        smTime::DoublePrec time		= timeSec + timeMSec ;

        return static_cast<smTime::DoublePrec>( time ) ;
#endif
	}
};
