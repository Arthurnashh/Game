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
	class VelocityController;
	class PositionController;

	class RTDYNAMICS_EXPORT LandingController : public RTD::BotControllerBase
	{
	public:

		smVec3r mdef_platformVelocity;

		//
		//	Dynamic parameters that define how the current maneuver should
		//	be performed.
		//
		struct Params
		{
			RTD::DynamicControllerParameterVec3r m_landingLocation;
			RTD::DynamicControllerParameterVec3r m_platformVelocity;
			RTD::DynamicControllerParameterReal  m_hoverHeight;
			RTD::DynamicControllerParameterReal	 m_approachSpeed;
			RTD::DynamicControllerParameterReal	 m_approachAngle;
			RTD::DynamicControllerParameterReal	 m_touchdownSpeed;
		};

		Params m_dynParams;

		//
		// Parameters that depend on the properties of the controlled vehicle
		//
		smCoefPos		mp_hoverLocationTolerance;
		smCoefPos		mp_hoverSpeedTolerance;

		RTD::PIDController*	mpid_verticalSpeed;
		RTD::PIDController*	mpid_distanceToHover;

		LandingController( RotorcraftBot* controlledBot , const std::string& name );
		virtual ~LandingController(void);

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );
	protected:
		enum MODE
		{
			APPROACH_TO_HOVER = 0,
			TOUCHDOWN
		};

		RTD::StateVariableReal	s_touchdownHeading;
		RTD::StateVariableUInt	s_mode;

		RTD::StateVariableRealDataHistory	s_landingLocationHistoryX;
		RTD::StateVariableRealDataHistory	s_landingLocationHistoryY;
		RTD::StateVariableRealDataHistory	s_landingLocationHistoryZ;

		

		virtual	void	updateForApproachToHover( float deltaTime );
		virtual	void	updateForTouchdown( float deltaTime );

		smVec3r			computeInterceptForApproach( double deltaTime );

		
	};


}
