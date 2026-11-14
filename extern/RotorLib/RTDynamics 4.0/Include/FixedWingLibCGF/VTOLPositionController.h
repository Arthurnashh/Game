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


namespace FixedWingLibCGF
{
	class AirplaneBot;

	/**	 
		VTOLPositionController is used to hover the airplane to a user defined position.

		Details of the controller can be found in the Programmer's Guide.
	*/ 	
	class RTDYNAMICS_EXPORT VTOLPositionController :	public RTD::BotControllerBase
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
			RTD::DynamicControllerParameterReal			m_desiredHeading;
		};

		Params m_dynParams;


		smReal	mp_distanceControlP; 
		smReal	mp_distanceControlD; 
		smReal	mp_altitudeControlP; 
		smReal	mp_altitudeControlD; 

		VTOLPositionController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~VTOLPositionController(void);

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );
	protected:
		
		virtual void	updateApproach(float deltaTime );
		virtual void	updateHover(float deltaTime );
		

	};
}

#endif