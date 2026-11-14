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

namespace RTD
{
	class PIDController;
}

namespace FixedWingLibCGF
{
	class AirplaneBot;

	/**
		Landing Controller implements landing maneuver.

		Details of the controller can be found in the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT VTOLLandingController :	public RTD::BotControllerBase
	{
	public:
		VTOLLandingController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~VTOLLandingController();

	        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
		struct Params
		{
			RTD::DynamicControllerParameterVec2r   m_landingLocationPosNM;
			RTD::DynamicControllerParameterReal    m_landingLocationAltitudeFeet;
			RTD::DynamicControllerParameterReal    m_hoverAltitudeFeet;	// Altitude above m_landingLocationAltitudeFeet
		};

		Params	m_dynParams;

		

		virtual void	initialize();
		virtual void	update( smReal deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		virtual void	computeLandingGeometry( smVec3r& touchdownPoint , smVec3r& hoverPoint , smVec3r& glidePathDir , smVec3r& nearestPtOnGlidePath  );

	protected:

		RTD::PIDController*	mpid_speedController;

		virtual void	updateApproachToHover( smReal deltaTime );
		virtual void	updateDescend( smReal deltaTime );
		virtual void	updateTouchdown( smReal deltaTime );

	};
}

#endif