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
#include "RTD/BotBase.h"

namespace RotorLibCGF
{
	class RotorcraftBot;

	class RTDYNAMICS_EXPORT ForceController : public RTD::BotControllerBase
	{
	public:
		//
		// Dynamic parameters that define how the current maneuver should be carried out
		//
		struct Params
		{
			RTD::DynamicControllerParameterVec3r	m_forceToApply;
			RTD::DynamicControllerParameterReal		m_heading;
		};

		Params m_dynParams;

		ForceController( RotorcraftBot* controlledBot , const std::string& name );
		virtual ~ForceController(void);

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual bool	readParamStr( const std::string& parameterString , std::string& error );
	protected:

	};

}
