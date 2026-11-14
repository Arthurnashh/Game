/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/WindBase.h"

namespace RTD
{
    class RTDYNAMICS_EXPORT Wind : public WindBase
	{
	public:
		Wind( const std::string& name, SimulationVariableUser* parent ) ;
		virtual ~Wind();

		virtual void reset();

		/** setWindVelocity allows specification of wind with direction and speed.
		*
		*	@param	direction		0 points to north (forward), counter clockwise is positive, in degrees
		*	@param	speed			horizontal wind speed in m/s
		*	@param	verticalSpeed	vertical wind speed in m/s
		*/
		virtual void setWindVelocity( smReal direction, smReal speed, smReal verticalSpeed );

		/** setWindVelocity can be used to specify wind vector in WCS frame. */
		virtual void setWindVelocityInWCS( const smVec3r& v);

		/** setWindVelocity can be used to specify wind vector in NED frame. */
		virtual void setWindVelocityInNED( const smVec3r& v);

		/** Return the current wind direction in radians. */
		virtual smReal		getWindDirection() const { return o_psiWind.getValue(); }

	protected:
		OutputVariableReal	o_psiWind;
	};
}
