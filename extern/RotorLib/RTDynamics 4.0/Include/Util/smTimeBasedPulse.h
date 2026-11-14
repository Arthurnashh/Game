/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


#include "Physics/smMath.h"
#include "Physics/smTypes.h"
#include <limits.h>

/**	
*	See smTimeBasedPulse.odg
*/
class smTimeBasedPulse
{
public:
	smReal	mWaitTime ;
	smReal  mPulseMagnitude ;
	int     mRepeat ;
	smReal  mPulseTime ;

	smReal	mTime ;
public:
	smTimeBasedPulse()
	{
		reset() ;
	}

	void reset()
	{
		mTime = 0;
		mWaitTime = 1 ;
		mPulseMagnitude = 1 ;
		mRepeat = INT_MAX ;
		mPulseTime = 1 ;
	}

	smReal	update( smTime stepsize )
	{
		mTime += stepsize ;

		if ( mTime < mWaitTime )
			return 0 ;
		else if ( mTime > mWaitTime )
		{
			smReal time = mTime - mWaitTime ;

			if ( time / (2.0f*mPulseTime) > mRepeat )
				return 0 ;

			time = fmod( time, 2.0f*mPulseTime );
			if ( time < mPulseTime )
				return mPulseMagnitude ;
			else
				return 0 ;
		}

		smValidateAndExit(false,"mTime<mWaitTime", "smTimeBasedPulse::update");		
		return  0 ;
	}
};

