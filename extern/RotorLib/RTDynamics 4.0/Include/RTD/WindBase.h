/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Base.h"
#include "RTD/ParamDb.h"

namespace RTD
{
	class RTDYNAMICS_EXPORT WindBase : public SimulationVariableUser
	{
	public:
		WindBase( const std::string& name, SimulationVariableUser* parent );

		virtual void update( float dt ) ;

		smVec3r getLinearWindVelocityNED() { return o_vWindNED.getValue(); }
		smVec3r getLinearWindVelocityWCS() { return o_vWindWCS.getValue(); }
		smVec3r getAngularWindVelocity() { return o_vWindPQR.getValue(); }

	protected:
		OutputVariableVec3r	o_vWindNED; // Linear wind velocities in NED frame. In m/s
		OutputVariableVec3r	o_vWindWCS; // Linear wind velocities in WCS frame. In m/s
		OutputVariableVec3r	o_vWindPQR;	// Angular wind velocities. In rad/s
	};
}
