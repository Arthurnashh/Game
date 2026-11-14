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
}

namespace FixedWingLibCGF
{
	class AirplaneBot;

	/**
		Landing Controller implements landing maneuver.

		Details of the controller can be found in the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT LandingController :	public RTD::BotControllerBase
	{
	public:
		LandingController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~LandingController();

	        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
		struct Params
		{
			RTD::DynamicControllerParameterVec3r   m_runwayStart;
			RTD::DynamicControllerParameterVec3r   m_runwayEnd;
			RTD::DynamicControllerParameterReal    m_approachSpeedKts;
			RTD::DynamicControllerParameterReal    m_desiredLoad;
			RTD::DynamicControllerParameterReal    m_glideSlope;
			RTD::DynamicControllerParameterReal    m_maxRollRate;			

			RTD::DynamicControllerParameterReal    m_descendRateFtMin;

		};

		Params	m_dynParams;		

		smReal	mp_wheelBrakeApplicationSpeedKts;
		smReal	mp_flapExtensionSpeedKts;

		virtual void	initialize();
		virtual void	update( smReal deltaTime );
		virtual void	finalize();

		virtual void	reset();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		virtual void	computeLandingGeometry();

	protected:
		smReal	m_turnRadiusApproach;
		smReal	m_turnRadiusCurrent;
		smVec3r m_loiterPointLeft;
		smVec3r m_loiterPointRight;	
		smVec3r m_middleOfLoiterPoints;	
		smVec3r	m_rightDir;
		smVec3r m_touchdownPoint;
		smVec3r m_glidePathDir;
		smVec3r m_nearestPtOnGlidePath;

		void	resetLandingGeomVars();


		virtual void	updateDescend( smReal deltaTime );
		virtual void	updateApproach( smReal deltaTime );
		virtual void	updateLandingRoll( smReal deltaTime );

	};
}

