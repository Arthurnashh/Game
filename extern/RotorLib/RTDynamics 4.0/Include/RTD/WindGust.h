/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/WindBase.h"

namespace RTD
{
	class RTDYNAMICS_EXPORT WindGust : public WindBase
	{
	public:
		WindGust( const std::string& name, SimulationVariableUser* parent ) ;
		virtual ~WindGust() ;

		/** Start a gust. 
		*	@param	startTime	time in seconds the gust model will wait to start the gust.
		*/
		virtual void start( smReal startTime = 0 );


		virtual void update( float deltaTime );

		/** Set the gust velocity
		*	@param	v	gust velocity in WCS
		*/
		void setVelocity( const smVec3r& vWCS);


		/** Set the gust distance
		*	@param	d	gust distance in meters
		*/
		void setDistance( const smReal& d);

		/** Set the current position of the aircraft/helicopter. This method shall be called
		*	every simulation step.
		*	@param	p	current position of the aircraft/helicopter
		*/
		void setCurrentPosition( const smVec3r& p) { i_curPos = p; }

	protected:
		InputVariableVec3r		i_vWCS;
		InputVariableReal		i_distance;
		StateVariableVec3r		i_curPos;

		StateVariableReal		s_simTime;
		StateVariableReal		s_startTime;
		StateVariableReal		s_distanceTravelled;
		StateVariableVec3r		s_lastPos;
		StateVariableBool		s_gustStarted;
	};
}
