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

	class RTDYNAMICS_EXPORT VelocityController :	public RTD::BotControllerBase
	{
	public:
		VelocityController( RotorcraftBot* controlledBot , const std::string& name );
		virtual ~VelocityController(void);

		//
		// Dynamic parameters that define how the current maneuver should be performed.
		//
		struct Params
		{
			RTD::DynamicControllerParameterBool			m_coordinatedTurnEnabled;
			RTD::DynamicControllerParameterVec3r		m_desiredVelocity;	// If the desired velocity > never exceed speed of the rotorcraft it will be automatically reduced.
			RTD::DynamicControllerParameterReal			m_CTdesiredTurnRadius;
			RTD::DynamicControllerParameterReal			m_CTLinearAccelCoeff;
			RTD::DynamicControllerParameterReal			m_CTVerticalAccelCoeff;
			RTD::DynamicControllerParameterReal			m_CTMaxCentrifugalAccel;
			RTD::DynamicControllerParameterReal			m_CTMaxLinAccel;
			RTD::DynamicControllerParameterReal			m_CTMaxVertAccel;
			RTD::DynamicControllerParameterReal			m_CTRollingSpeed;
			RTD::DynamicControllerParameterReal			m_CHdesiredHeading;
			RTD::DynamicControllerParameterReal			m_CHMaxSidewardAccel;
			RTD::DynamicControllerParameterReal			m_CHMaxForwardAccel;
			RTD::DynamicControllerParameterReal			m_CHMaxVertAccel;
			RTD::DynamicControllerParameterReal			m_weaveFrequencyWeavePerMin;
			RTD::DynamicControllerParameterReal			m_weaveAmplitude;
		};

		Params	m_dynParams;
	
		//
		// Parameters that depend on the properties of the controlled vehicle
		//
		smCoefPos		mp_C1;
		smCoefPos		mp_C2;
		smCoefPos		mp_C3;
		smCoefPos		mp_C4;

		smCoefPos		mp_constHeadingSidewardSpeedContrP;
		smCoefPos		mp_constHeadingForwardSpeedContrP;
		smCoefPos		mp_constHeadingVerticalSpeedContrP;

		smCoefPos		mp_autoHeadingSpeedContrP;
		smCoefPos		mp_autoHeadingVerticalSpeedContrP;
		RTD::PIDController*	mpid_turnController;


		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );
	protected:

		enum State
		{
			ST_NONE = 0,
				ST_DECISION,
				ST_STABILIZE,
				ST_TURN,
				ST_CONSTHDGVHOLD
		};

		RTD::StateVariableUInt			s_currentState;
		RTD::StateVariableUInt			s_previousState;
		RTD::StateVariableReal			s_hdgAtStateChange;
		RTD::StateVariableVec3r			s_VatStateChange;
		RTD::StateVariableReal			s_prevDesiredCentripedalAcc;	// Desired centripedal accel in previous frame
		RTD::StateVariableDouble		s_weaveTimer;

		smVec3r	m_HPR;		//  Current attitude
		smVec3r	m_rotVel;	//  Current rotational velocity
		smVec3r	m_V;		//	Current velocity
		smVec3r	m_Vgr;		//	Current ground velocity
		smVec3r	m_Vd;		//	Desired velocity
		smVec3r	m_Vgrd;		//	Desired ground velocity

		smReal	m_DOM;		//	Direction of motion ( heading of m_Vgr vector )
		smReal	m_DOMd;		//	Desired direction of motion ( heading of m_Vgrd	}

		smVec3r	m_FW;		//  Forward vector
		smVec3r	m_SW;		//  Sideward vector

		smVec3r	m_FWgr;		//  Ground forward vector
		smVec3r	m_SWgr;		//  Ground sideward vector


		

		//
		// State machine state update methods
		//
		virtual	void	executeDecision( smReal deltaTime );
		virtual	void	executeStabilize( smReal deltaTime );
		virtual	void	executeConstHDGVHold( smReal deltaTime );
		virtual	void	executeTurn( smReal deltaTime );



		void			constHeadingVHold(	const smVec3r& Vd , smReal HEd , smReal deltaTime ,
											smVec3r& outForce ,smReal& outHeading );

		void			coordinatedTurnVHold(	const smVec3r& Vd , smReal deltaTime ,
												smVec3r& outForce ,smReal& outHeading );

		//
		// Utilities
		//
		virtual	void	getGravitationalForce( smVec3r& g );
		virtual void	getAirDrag( smVec3r& d );
		virtual	void	updateVehicleStatus();
	};
}
