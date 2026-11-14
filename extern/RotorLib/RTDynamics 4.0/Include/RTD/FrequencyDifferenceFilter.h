/*
(c) Copyright 2010, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Common.h"


namespace RTD
{

	class RTDYNAMICS_EXPORT FrequencyDifferenceFilter : public SimulationVariableUser
	{
	public:
		FrequencyDifferenceFilter( const std::string& name , SimulationVariableUser* parent );

		virtual ~FrequencyDifferenceFilter();

		virtual smVec3r update( const smVec3r& data  , const smVec3r& dataDerivative, const smVec3r& secondDataDerivative , const smDouble& deltaTime );
		virtual smVec3r update( const smVec3r& data , const smDouble& deltaTime );

		/** Returns the data point which is computed at the last update(). */
		const smVec3r&	getCurrentFilteredData() const					{	return s_currentData.getValue();	}

		const smVec3r&	getCurrentNotFilteredData() const				{	return s_prevData.getValue();	}

		virtual void reset();

	protected:
		RTD::StateVariableDouble s_time;
		RTD::StateVariableVec3r  s_prevData;
		RTD::StateVariableVec3r	 s_currentData;
		RTD::StateVariableVec3r  s_lastKnownNewData;
		RTD::StateVariableVec3r  s_lastKnownNewDataDeriv;
		RTD::StateVariableVec3r  s_lastKnownNewDataSecondDeriv;
		RTD::StateVariableDouble s_lastKnownNewDataTime;
	};
}