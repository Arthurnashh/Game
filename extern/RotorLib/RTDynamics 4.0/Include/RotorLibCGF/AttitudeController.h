/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once
#include "RotorcraftBot.h"
#include "RTD/PIDController.h"


namespace RotorLibCGF
{
	class RTDYNAMICS_EXPORT AttitudeController :	public RTD::BotControllerBase
	{
	public:
		//
		// Dynamic parameters that define how the current maneuver should be carried out
		//
		struct Params
        {
			RTD::DynamicControllerParameterVec3r	m_desiredAttitude;
			RTD::DynamicControllerParameterReal		m_collective;
		};

		Params	m_dynParams;

		//
		// Parameters that depend on the properties of the controlled vehicle
		//
		RTD::PIDController*	mpid_heading;
		RTD::PIDController*	mpid_pitch;
		RTD::PIDController*	mpid_roll;

		AttitudeController( RotorcraftBot* controlledBot , const std::string& name );
		virtual ~AttitudeController(void);


		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );
	protected:



//		virtual	bool	acceptCommand( const std::string& commandString );
	};

}
