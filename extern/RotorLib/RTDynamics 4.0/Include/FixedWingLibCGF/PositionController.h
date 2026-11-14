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


namespace FixedWingLibCGF
{
	class AirplaneBot;

	/**	 
		PositionController is used to fly the airplane to a user defined position.

		Details of the controller can be found in the Programmer's Guide.
	*/ 	
	class RTDYNAMICS_EXPORT PositionController :	public RTD::BotControllerBase
	{
	public:


        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
		struct Params
		{
			RTD::DynamicControllerParameterVec2r		m_desiredPositionNM;
			RTD::DynamicControllerParameterReal			m_desiredAltitudeFeet;
			RTD::DynamicControllerParameterReal			m_desiredSpeedKts;
			RTD::DynamicControllerParameterReal			m_desiredVerticalSpeedFtMin;
			RTD::DynamicControllerParameterReal			m_desiredLoadFactor;
			RTD::DynamicControllerParameterReal			m_desiredRollSpeed;
			RTD::DynamicControllerParameterBool			m_useAirBrakes;
			RTD::DynamicControllerParameterBool			m_enableTerrainFollowingFlight;
			RTD::DynamicControllerParameterBool			m_avoidInvertedFlight;
			RTD::DynamicControllerParameterReal			m_weaveFrequencyWeavePerMin;
			RTD::DynamicControllerParameterReal			m_weaveAmplitude;
		};

		Params m_dynParams;


		PositionController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~PositionController(void);

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );
	protected:
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


		unsigned int		m_firstDataLoggerChannelIndex;


		virtual	void	updateVehicleStatus();

		

	};
}

