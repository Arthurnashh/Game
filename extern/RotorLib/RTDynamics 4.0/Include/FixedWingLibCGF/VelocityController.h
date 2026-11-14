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
		Velocity Controller is used to reach and maintain a user defined velocity vector. Velocity Controller can perform the 
		maneuvers to reach the desired velocity vector with user defined parameters such as a user defined load factor.

		Details of the controller can be found in the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT VelocityController :	public RTD::BotControllerBase
	{
	public:
		VelocityController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~VelocityController(void);

		enum Mode
		{
			WINGS_LEVEL = 0, 
				TWO_DIMENSIONAL_TURN,
				THREE_DIMENSIONAL_TURN
		};


        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
		struct Params
		{
			RTD::DynamicControllerParameterVec3r			m_desiredVelocity;
			RTD::DynamicControllerParameterReal				m_desiredRollSpeed;
			RTD::DynamicControllerParameterBool				m_constantRadiusMode;
			RTD::DynamicControllerParameterReal				m_constRDesiredRadius;
			RTD::DynamicControllerParameterReal				m_constRLoadLimit;
			RTD::DynamicControllerParameterReal				m_constLoadDesiredLoad;
			RTD::DynamicControllerParameterVec3r			m_preferredUpDirection;
			RTD::DynamicControllerParameterEnum<Mode>		m_mode;			
			RTD::DynamicControllerParameterBool				m_useAirBrakes;			
			RTD::DynamicControllerParameterReal				m_thrustVectoringAoALimit;
			RTD::DynamicControllerParameterReal				m_thrustVectoringSpeedLimitKts;					
			RTD::DynamicControllerParameterReal				m_weaveFrequencyWeavePerMin;
			RTD::DynamicControllerParameterReal				m_weaveAmplitude;
		};

		Params m_dynParams;

		//
		// Parameters that depend on the properties of the controlled vehicle
		//
		smReal		mp_liftControllerMixingThreshold;

		// Controllers
		RTD::PIDControllerD*	mpid_lift;
		RTD::PIDController*		mpid_rollController;
		RTD::PIDController*		mpid_verticalSpeedController;		
		RTD::PIDController*		mpid_directionController;



		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		const smVec3r&	getDesiredLift() const {	return c_Ldwcs; }

		smVec3r			getMaxVerticalForce() const	{	return c_Fverticalmax;	}

	protected:		
		unsigned int		m_firstDataLoggerChannelIndex;

		SpeedController		m_speedController;

		smVec3r	m_HPR;		//  Current attitude
		smVec3r	m_rotVel;	//  Current rotational velocity
		smVec3r	m_V;		//	Current velocity
		smVec3r	m_Vgr;		//	Current ground velocity

		smVec3r	m_Vd;		//	Desired velocity
		smVec3r	m_Vguide;	//	Guide velocity		
		smVec3r	m_Vguidegr;	//	Guide ground velocity		

		smReal	m_S;		//  Speed in m/s
		smVec3r	m_Vdir;		//  Flight direction vector (normalized)
		smReal	m_DOM;		//	Direction of motion ( heading of m_Vgr vector )

		smVec3r	m_FW;		//  Forward vector
		smVec3r	m_SW;		//  Sideward vector
		smVec3r	m_UW;		//  Upward vector

		smVec3r	m_FWgr;		//  Ground forward vector
		smVec3r	m_SWgr;		//  Ground sideward vector

		smMatrix c_bodyXform;
		smMatrix c_invBodyXform;
		smVec3r	c_posLiftDirWCS; // A unit vector in WCS which shows the direction of positive lift vector
		smVec3r	c_posLiftDirBCS; // A unit vector in BCS which shows the direction of positive lift vector

		smVec3r	c_Ldwcs; // desired lift in world coordinates
		smVec3r	c_Ldbcs; // desired lift in body coordinates

		RTD::StateVariableReal					s_Edir;
		RTD::StateVariableRealDataHistory		s_EdirHistory;

		RTD::StateVariableDouble				s_Eroll;
		RTD::StateVariableDoubleDataHistory		s_ErollHistory;

		RTD::StateVariableDouble				s_Eclift;
		RTD::StateVariableDoubleDataHistory		s_EcliftHistory;

		RTD::StateVariableReal					s_Evertical;
		RTD::StateVariableRealDataHistory		s_EverticalHistory;

		RTD::StateVariableRealDataHistory		s_desiredVelocityDirectionHistory;
		RTD::StateVariableRealDataHistory		s_desiredVelocityDirectionDerivativeHistory;
	

		RTD::StateVariableDouble				s_weaveTimer;

		smVec3r	c_Fdhorizontal;
		smVec3r	c_Fdvertical;
		smVec3r	c_Fd;

		smVec3r	c_Fverticalmax;

		smReal	c_Fturnradius;
		smReal	c_Floadlimit;
		smReal	c_Fdesiredload;

		virtual void	computeThrustWithThrustVectoring( smVec3r& Tdwcs , smReal& airbrakes , smReal& throttle , smReal& nozzleDirection , smReal deltaTime );
		virtual void	computeThrust( smVec3r& Tdwcs , smReal& airbrakes , smReal& throttle , smReal deltaTime );

		virtual smVec3r	computeTurningForceDirection( const smVec3r& Vcurrent, const smVec3r& Vdesired );

		virtual smDouble computeRollError( const smVec3d& Ldbcs );
		virtual smDouble computeLiftCoeffError( const smVec3r& Ldwcs , const smVec3r& Lbcs,const smDouble& Eroll, smReal loadLimit , bool allowNegativeLoads );
		virtual smVec3r	computeDesiredLift( const smVec3r& Td , float deltaTime );
		virtual smVec3r	computeDesiredLiftSeparateVertical( const smVec3r& Td , bool forceWingsLevel , float deltaTime );
		virtual smReal  computeTurnForceMagnitudeForGivenLoad( smReal load , const smVec3r& turnForceDirWCS , const smVec3r& vertForceWCS = smVec3r(0.0f,0.0f,0.0f) );

		virtual	void	updateVehicleStatus();

		smReal			getEffectiveDesiredRollRate();
		smReal			getEffectiveConstLoadDesiredLoad();
		smReal			getEffectiveConstRadiusLoadLimit();


	};
}
