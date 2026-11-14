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

/**	smDeadBandFilter is a simple dead band filter to be used with input devices.
*		The deadband is defined around the given origin value.
*/
class smDeadBandFilter
{
public:
	// @deprecated Please use the version below for future work.
	static void filter( smReal& inValue, const smReal& deadBand, const smReal& clampValue )
	{
		if ( inValue > deadBand / 2.0f )
		{
			inValue = (inValue * 2.0f) / ( 2.0f - deadBand ) + deadBand / ( deadBand - 2.0f );
			return;
		}

		if ( inValue < -deadBand / 2.0f )
		{
			inValue = (inValue * 2.0f) / ( 2.0f - deadBand ) + deadBand / ( 2.0f - deadBand );
			return;
		}

		inValue = clampValue;
		
	}

	/** Deadband filter for use with input devices.
	*       @param	in			value to be filtered
	*		@param	r			deadband range is defined as [-r,+r]. Hence r is in [0,inf]
	*	    @param  max			maximum value of input function [0,inf]
	*	    @param  clampValue	value to be returned when in is between [-r,+r]
	*/
	// See DinaPhysics/Doc/smFilter.odg
	// See testDeadBandFilter in DinaPhysicsTests
	static void filter( smReal& in, const smReal& r, const smReal& max, const smReal& clampValue )
	{
		if ( -r <= in && in <= r )
		{
			in = clampValue ;
			return ;
		}

		smReal m = max / (max - r) ;

		if ( in < -r )	in = m * (in + r ) ;
		if ( in > +r )	in = m * (in - r ) ;
	}
};

/**	Limit the input signal to the upper and lower saturation values.
*/
class smSaturationFilter
{
public:
	static void filter( smReal& inValue, const smReal& lower, const smReal& upper )
	{
		if ( inValue > upper )
			inValue = upper ;

		if ( inValue < lower)
			inValue = lower ;
	}
};

template <class Obj>
class smLagFilter
{
public:
	Obj	m_prevValue ;

	smLagFilter() : m_prevValue(0.0f) 
	{
	};

	void setPrevValue( smReal val )
	{ m_prevValue = val ;}

	/**	
	*	@param	lagFactor	[0,1]	0 lag means new values are immediately effective
	*								smaller values mean new values will be mixed with
	*								previous values.
	*/
	// The in param is intentionally not a reference. This allows calling this method like:
	//    filter ( a, a, 0.5f ) ;
	void filter( Obj in, Obj& out, smReal lagFactor )
	{
		lagFactor = smMath::clamp(lagFactor, 0.0f, 1.0f) ;
		out = in * ( 1.0f - lagFactor ) + m_prevValue * lagFactor ;

		m_prevValue = out ;
	}
};

/** Similar to the lag filter. Allows smooth transition of values 
*	across the thresholds.
*/
class smSmoothThresholdFilter
{
	int	m_countDownCount ;
	int	m_countDownTimer ;

	bool m_prevFlag ;

public:
	smSmoothThresholdFilter()
	{
		reset() ;
	}

	void start( int count )
	{
		m_countDownCount = count ;
		m_countDownTimer = count ;
	}

	void reset()
	{
		m_countDownTimer = -1 ;
		m_prevFlag = false;
	}

	bool isRunning()
	{
		return m_countDownTimer > 0 ;
	}

	void filter( bool flag, int noTransSteps, smReal in, smReal in2, smReal& out )
	{
		if ( m_prevFlag != flag )
			start( noTransSteps );

		if ( isRunning() )
		{
			smReal mixFactor ;
			mixFactor = smReal(m_countDownTimer) / m_countDownCount ;
			out = in * ( 1.0f - mixFactor ) + in2 * mixFactor ;

			m_countDownTimer--;
		}

		m_prevFlag = flag ;
	}
};