/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RotorLib
Subsystem:		RLDynamics

The content of this work is propriatery and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#ifdef WIN32
#include <windows.h>
#endif

#include <deque>
#include "Util/smPrinter.h"

/**
*	smTimerClock allows finding of bottlenecks in simulation code.
*	It allows nested blocks. Use it to identify really easy bottlenecks
*
*	Limitations :
*		1. It is not precise
*		2. It does its own printing so it is not suitable for fine tuning.
*/
class smTimerClock
{
protected:
	class TimeStamp
	{
	public:
		std::string	mBlockName ;
		double		mTime ;
	};

	std::deque<TimeStamp>	mTimes ;
	smPrinter*				mPrinter ;

public:
	smTimerClock( smPrinter* pr ): mPrinter( pr )
	{}

	/** Use to start a new block. The name is a simply identifier for the block.	*/
	void	push( const std::string& blockName )  ;

	/** Use to finish a block. The last pushed block will be popped. */
	void pop( )  ;
};

