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
#include "FixedWingLibCGF/Util.h"


namespace FixedWingLibCGF
{
	class AirplaneBot;

	/**
		Basic Low Level Controller is designed to perform low level piloting tasks such as maintaining a certain pitch and roll attitude or
		performing a nose pull-up with a given load factor.

		Details of the controller can be found in the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT BasicLowLevelController :	public RTD::BotControllerBase
	{
	public:

		enum LongitudinalControlMode
		{
			DesiredLoadFactor	=	0,
			DesiredPitch,
			DirectElevatorControl
		};

		enum LateralControlMode
		{
			DesiredRoll	=	0,
			DirectAileronControl
		};

		enum SpeedControlMode
		{
			DesiredSpeed  = 0,
			DirectThrottleAndAirbrakeControl
		};


        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
		struct Params
		{
			RTD::DynamicControllerParameterEnum<LongitudinalControlMode>	m_longitudinalControlMode;

			// Desired pitch mode parameters
			RTD::DynamicControllerParameterReal						m_DPM_desiredPitch;
			RTD::DynamicControllerParameterReal						m_DPM_desiredPitchRate;

			// Desired load factor mode parameters
			RTD::DynamicControllerParameterReal						m_DLFM_desiredLoadFactor;

			// Direct longitudinal control mode parameters
			RTD::DynamicControllerParameterReal						m_DLCM_elevator;
			

			RTD::DynamicControllerParameterEnum<LateralControlMode>			m_lateralControlMode;

			// Desired roll mode parameters
			RTD::DynamicControllerParameterReal						m_DRM_desiredRoll;			
			RTD::DynamicControllerParameterReal						m_DRM_desiredRollRate;

			// Direct lateral control mode parameters
			RTD::DynamicControllerParameterReal						m_DLCM_aileron;		

						
			RTD::DynamicControllerParameterEnum<SpeedControlMode>	m_speedControlMode;
			
			// Desired speed mode parameters
			RTD::DynamicControllerParameterReal				m_DSM_desiredSpeed;
			RTD::DynamicControllerParameterBool				m_DSM_useAirBrake;

			// Direct thrust control mode parameters
			RTD::DynamicControllerParameterReal				m_DTC_throttle;	
			RTD::DynamicControllerParameterReal				m_DTC_airbrake;
		};

		Params	m_dynParams;

		//
		// Parameters that depend on the properties of the controlled vehicle
		//
		RTD::PIDController*	mpid_pitch;
		RTD::PIDController*	mpid_roll;
		RTD::PIDController*	mpid_loadFactor;

		BasicLowLevelController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~BasicLowLevelController(void);


		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );
	protected:

		SpeedController	m_speedController;

		virtual void	resetLoadFactorPID();

	};

}
