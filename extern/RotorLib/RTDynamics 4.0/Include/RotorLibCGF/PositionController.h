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
#include "RotorcraftBot.h"


namespace RTD
{
	class TerrainQueryLoadBalancer;
}

namespace RotorLibCGF
{

	class RTDYNAMICS_EXPORT PositionController :	public RTD::BotControllerBase
	{
	public:
		//
		// Dynamic parameters that define how the current maneuver should be performed.
		//
		class Params
        {
        public:
			RTD::DynamicControllerParameterBool			m_coordinatedTurnEnabled;
			RTD::DynamicControllerParameterVec2r		m_desiredPosition;
			RTD::DynamicControllerParameterReal			m_desiredAltitude;
			RTD::DynamicControllerParameterReal			m_minAltitudeAboveGround;
			RTD::DynamicControllerParameterReal			m_desiredSpeed;
			RTD::DynamicControllerParameterReal			m_desiredVerticalSpeed;
			RTD::DynamicControllerParameterReal			m_CHdesiredHeading; // Desired heading of the vehicle in case coordinated turn is off
			RTD::DynamicControllerParameterBool			m_dontStopEnabled; // If enabled the vehicle will *not* stop when it arrives the desired position.
			RTD::DynamicControllerParameterBool			m_terrainFollowingEnabled;
			RTD::DynamicControllerParameterReal   		m_terrainQueryInterval;
			RTD::DynamicControllerParameterReal			m_weaveFrequencyWeavePerMin;
			RTD::DynamicControllerParameterReal			m_weaveAmplitude;
		};

		Params   m_dynParams ;

	

		//
		// Parameters that depend the on properties of the controlled vehicle
		//
		smCoefPos		mp_distanceControlP;
		smCoefPos		mp_distanceControlD;
		smCoefPos		mp_distanceToTurnRadiusRatio;
		smCoefPos		mp_C1;
		smCoefPos		mp_C2;
		smCoefPos		mp_altitudeControlP;
		smCoefPos		mp_altitudeControlD;
		smCoefPos		mp_C3;
		smCoefPos		mp_C4;
		smCoefPos		mp_maxAngularSpeed;


		PositionController( RotorcraftBot* controlledBot , const std::string& name );
		virtual ~PositionController(void);

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );
	protected:
		enum State
		{
			ST_NONE = 0,
				ST_CRUISE,
				ST_STOP,
				ST_CONSTHMOVETO
		};

		smVec3r m_HPR;		//  Current orientation
		smVec3r	m_V;		//	Current velocity
		smVec3r	m_Vgr;		//	Current ground velocity

		smVec3r	m_XYZ;		//  Current position

		smVec3r	m_D;		//	Displacement vector ( m_desiredPosition - m_XYZ )
		smVec3r	m_Dgr;		//	Ground displacement vector;

		smVec3r	m_FW;		//  Forward vector
		smVec3r	m_SW;		//  Sideward vector

		smVec3r	m_FWgr;		//  Ground forward vector
		smVec3r	m_SWgr;		//  Ground sideward vector

		smReal	m_DOM;		//	Direction of motion ( heading of m_Vgr vector )

		RTD::TerrainQueryLoadBalancer*	m_tqLoadBalancer;

		RTD::StateVariableReal		s_desiredAltitudeInternal;
		RTD::StateVariableReal		s_desiredSpeedInternal;		

		RTD::StateVariableUInt		s_currentState;
		RTD::StateVariableUInt		s_previousState;

		RTD::StateVariableReal		s_hdgAtStateChange;
		RTD::StateVariableVec2r		s_prevDesiredPosition;

		

		//
		// Update methods for state machine states
		//
		virtual	void	executeCruise( smReal deltaTime );
		virtual void	executeStop( smReal deltaTime );
		virtual void	executeConstHdgMoveTo( smReal deltaTime );

		//
		// Utilities
		//
		virtual smReal	isFront( smReal FWtoD , smReal VtoD ) const;
		virtual	void	updateVehicleStatus();

		//  Needs to be called after  updateVehicleStatus()                                                          */
		virtual void	computeTerrainFollowingParams(	smReal altitudeAboveGround ,
														smReal speedWOObstacle,
														smReal& desiredAltitude ,
														smReal& desiredSpeed ) const;

		
	};
}

