/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RotorLibCGF/Common.h"

namespace RTD
{
	class PIDController;
}

namespace FixedWingLibCGF
{
	class AirplaneBot;

	/**
		TakeoffController Controller implements a take off maneuver.

		Details of the controller can be found in the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT TakeoffController :	public RTD::BotControllerBase
	{
	public:
		TakeoffController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~TakeoffController();

	        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
		struct Params
		{
			RTD::DynamicControllerParameterVec3r  m_runwayEnd;

			RTD::DynamicControllerParameterReal   m_desiredHeading;
			RTD::DynamicControllerParameterReal   m_desiredSpeedKts;
			RTD::DynamicControllerParameterReal   m_desiredAltitudeFeet;			
			RTD::DynamicControllerParameterReal   m_desiredClimbRateFeetMin;			
			RTD::DynamicControllerParameterReal   m_desiredLoad;
			RTD::DynamicControllerParameterReal   m_desiredRollRate;
			

		};

		Params	m_dynParams;

		smReal	mp_rotationSpeedKts;
		smReal	mp_rotationPitch;

		virtual void	initialize();
		virtual void	update( smReal deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );


	protected:
		smReal			computeRunwayEndBearing();
		virtual void	updateAccelerate( smReal deltaTime );
		virtual void	updateClimb( smReal deltaTime );

	};
}
