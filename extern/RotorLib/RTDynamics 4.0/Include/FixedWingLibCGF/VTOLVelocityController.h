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
#include "FixedWingLibCGF/Util.h"


namespace FixedWingLibCGF
{
	class AirplaneBot;

	/**
		Velocity Controller is used to reach and maintain a user defined velocity vector. Velocity Controller can perform the 
		maneuvers to reach the desired velocity vector with user defined parameters such as a user defined load factor.

		Details of the controller can be found in the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT VTOLVelocityController :	public RTD::BotControllerBase
	{
	public:
		VTOLVelocityController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~VTOLVelocityController(void);

	

        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
		struct Params
		{
			RTD::DynamicControllerParameterVec3r			m_desiredVelocity;
			RTD::DynamicControllerParameterReal				m_desiredHeading;
		};

		Params m_dynParams;

		smReal mp_verticalSpeedContrP;
		smReal mp_sidewardSpeedContrP;
		smReal mp_forwardSpeedContrP;
		smReal mp_headingContrP;
		smReal mp_headingContrD;

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );


	protected:		
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


		virtual	void	updateVehicleStatus();
		void			constHeadingVHold(	const smVec3r& Vd , smReal HEd , smReal deltaTime ,
											smVec3r& outForce ,smReal& outHeading );

	};
}

#endif