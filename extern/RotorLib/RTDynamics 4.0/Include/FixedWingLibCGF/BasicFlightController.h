/*
(c) Copyright 2008, RTDynamics
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
	class TerrainQueryLoadBalancer;
}

namespace FixedWingLibCGF
{
	class AirplaneBot;

	/**
		Basic Flight Controller implements basic flight maneuvers such as straight and level flight, climb, dive, and coordinated turn.

		Details of the controller can be found in the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT BasicFlightController :	public RTD::BotControllerBase
	{
	public:
		BasicFlightController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~BasicFlightController();

	        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
		struct Params
		{
			RTD::DynamicControllerParameterReal    m_desiredDirection;
			RTD::DynamicControllerParameterReal    m_desiredSpeedKts;
			RTD::DynamicControllerParameterReal    m_desiredAltitudeFeet;
			RTD::DynamicControllerParameterReal    m_desiredVerticalSpeedFtMin;
			RTD::DynamicControllerParameterReal    m_desiredLoad; 
			RTD::DynamicControllerParameterReal    m_desiredRollSpeed;	
			RTD::DynamicControllerParameterBool    m_useAirBrakes;
			RTD::DynamicControllerParameterVec3r   m_preferredUpDirection;	
			RTD::DynamicControllerParameterBool	   m_enableTerrainFollowingFlight;
			RTD::DynamicControllerParameterReal	   m_terrainQueryInterval;
			RTD::DynamicControllerParameterReal	   m_weaveFrequencyWeavePerMin;
			RTD::DynamicControllerParameterReal	   m_weaveAmplitude;
			
			RTD::DynamicControllerParameterReal    m_altitudeTolerance;
			RTD::DynamicControllerParameterReal    m_directionTolerance;
			RTD::DynamicControllerParameterReal    m_speedTolerance;
		};

		Params	m_dynParams;


		RTD::PIDController*	mpid_altitudeController;

		smReal				mp_overshootEstimateMultiplier;
		smVec3r				mp_TFFSmoothingFilterParams;

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );

	
	protected:
		smVec3r m_HPR;		//  Current orientation
		smVec3r	m_V;		//	Current velocity
		smVec3r	m_Vgr;		//	Current ground velocity

		smVec3r	m_XYZ;		//  Current position

		smVec3r	m_FW;		//  Forward vector
		smVec3r	m_SW;		//  Sideward vector

		smVec3r	m_FWgr;		//  Ground forward vector
		smVec3r	m_SWgr;		//  Ground sideward vector

		smReal	m_DOM;		//	Direction of motion ( heading of m_Vgr vector )
		
		RTD::TerrainQueryLoadBalancer*	m_tqLoadBalancer;

		virtual	void	updateVehicleStatus();

		virtual void computeTerrainFollowingParams(		smReal altitudeAboveGround,
														smReal speedWOObstacle,
														smReal& desiredAltitude ,
														smReal& desiredSpeed ) const;

		RTD::StateVariableReal				s_verticalSpeedMixFactor;
		RTD::StateVariableRealDataHistory	s_desiredAltitudeHistory;
		RTD::StateVariableRealDataHistory	s_desiredAltitudeAverageHistory;
		RTD::StateVariableReal				s_desiredAltitudeInternal;
		RTD::StateVariableBool				s_isDesiredAltitudeSpecified;		
		
		
		RTD::StateVariableReal				s_prevTFAltitude;
		RTD::StateVariableReal				s_TFFSmoothingFilterV;
	};
}

