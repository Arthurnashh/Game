/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

#include "RotorLibCGF/Common.h"

namespace RTD
{
	class PIDController;
}

namespace FixedWingLibCGF
{
	class AirplaneBot;

	/**
		VTOLTakeoffController Controller implements a vertical take off maneuver.

		Details of the controller can be found in the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT VTOLTakeoffController :	public RTD::BotControllerBase
	{
	public:
		VTOLTakeoffController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~VTOLTakeoffController();

	        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
		struct Params
		{
			RTD::DynamicControllerParameterReal   m_directionToFly;

			RTD::DynamicControllerParameterReal   m_desiredSpeedKts;
			RTD::DynamicControllerParameterReal   m_desiredAltitudeFeet;			
			RTD::DynamicControllerParameterReal   m_desiredLoad;
			RTD::DynamicControllerParameterReal   m_desiredRollRate;
			

		};

		Params	m_dynParams;

		smReal	mp_altitudeToStartAccelerationFeet;
		smReal	mp_cruiseModeSpeedKts;

		virtual void	initialize();
		virtual void	update( smReal deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );


	protected:

		virtual void	updateAscendAndTurn( smReal deltaTime );
		virtual void	updateAccelerate( smReal deltaTime );

	};
}

#endif