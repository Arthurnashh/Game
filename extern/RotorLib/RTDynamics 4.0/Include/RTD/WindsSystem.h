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
#include "RTD/WindGust.h"
#include "RTD/WindTurbulence.h"
#include "RTD/Wind.h"

namespace RTD
{
    
 	/** The WindsSystem is designed to be used with a vehicle or with any
		 wing element.
	*/
	class RTDYNAMICS_EXPORT WindsSystem
	{
	public:
		WindsSystem( const std::string& name , VehicleBase* vehicle);
		virtual ~WindsSystem() {};

		virtual void reset();

		virtual void update( smTime stepTime, smVec3r& vWindWCS, smVec3r& vWindPQR );

		Wind*           getMeanWind()   { return m_meanWind; }
		WindTurbulence* getTurbulence() { return m_turbulence; }
		WindGust*		getGust()		{ return m_gust; }

	protected:
		WindTurbulence* m_turbulence;
		Wind*	        m_meanWind;
		WindGust*		m_gust;
	};
}
