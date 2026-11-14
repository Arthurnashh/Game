/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include "Physics/smTypes.h"

/**
*	Simple utility class
*/
class smFrequencyClock
{
public:
	/**
	*	Call once at init time with the current time.
	*/
	void	init( smTime currentTime )
	{
		m_lastTimeStamp	=	currentTime ;
	}

	/**
	*	Returns true at the given frequency
	*/
	bool	check( smTime currentTime )
	{
		if ( ( currentTime - m_lastTimeStamp ) < m_period )
			return false ;
		else
		{
			m_lastTimeStamp = currentTime ;
			return true ;
		}
	}

	void setFrequency( smFrequency freq )
	{
		m_period = freq.toPeriod ();
	}

private:
	smTime		m_period ;
	smTime		m_lastTimeStamp ;
};

