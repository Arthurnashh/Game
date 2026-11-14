/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RotorLibCGF/Common.h"


namespace RotorLibCGF
{
	class RotorcraftBot;

	class RTDYNAMICS_EXPORT StopController :	public RTD::BotControllerBase
	{
	public:
		enum State
		{
			ST_NONE = 0,
			ST_STOP,
			ST_HOVER
		};

		//
		// Dynamic parameters that define how the current maneuver should be carried out
		//
		class Params
        {
        public:
            RTD::DynamicControllerParameterReal                     mdp_angleToTurn ;
			RTD::DynamicControllerParameterBool						mdp_stopAtGivenAltitude;
            RTD::DynamicControllerParameterReal                     mdp_altitudeToStop ;
            RTD::DynamicControllerParameterReal                     mdp_turningSpeedThreshold ;
            RTD::DynamicControllerParameterReal                     mdp_rotationalDecelerationScale ;
            RTD::DynamicControllerParameterReal                     mdp_translationalDecelerationScale ;			
        };

		Params   m_dynParams ;

		//
		// Parameters that depend on the properties of the controlled vehicle
		//
		smReal		mp_speedToAssumeStationary;
		smReal		mp_speedHysteresis;
		smReal		mp_rotSpeedToAssumeStationary;
		smReal		mp_rotSpeedHysteresis;

		smReal		mp_rotationalDecelerationCoefficient;
		smReal		mp_translationalDecelerationCoefficient;



		StopController( RotorcraftBot* controlledBot , const std::string& name );

		virtual ~StopController(void);

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );
	protected:
		RTD::StateVariableReal			s_altitudeToStopAuto;
		RTD::StateVariableReal			s_headingToStop;
		RTD::StateVariableVec2r			s_positionToStop;
		RTD::StateVariableUInt			s_currentState;


		//
		// State machine state update methods
		//
		virtual	void	executeStopState( smReal deltaTime );
		virtual	void	executeHoverState( smReal deltaTime );


		//
		// Utilities
		//
		virtual	void	getGravitationalForce( smVec3r& g );
		virtual void	getAirDrag( smVec3r& d );



	};
}
