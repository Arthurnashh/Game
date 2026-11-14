/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/FuelConsumptionModelBase.h"

namespace RotorLibCGF
{
	class SimplifiedRotorcraft;

	class RTDYNAMICS_EXPORT FuelConsumptionModel100 : public RTD::FuelConsumptionModelBase
	{

	public:
		class RTDYNAMICS_EXPORT ConfigParams
		{
		public:
			float	m_fuelAvailable;
			float	m_fuelConsDueToCollPerSecond;
		};

	protected:
		ConfigParams m_configParams;
		SimplifiedRotorcraft* vehicle;

		float	m_fuelConsumption;
		float	m_fuelAvailable ;

	public:

		FuelConsumptionModel100(SimplifiedRotorcraft* v) ;

		// Begin -- Reimplement FuelConsumptionModelBase methods
		virtual void update(smTime timestep);
		virtual void reset() ;
		virtual float getFuelConsumption();
		virtual void setFuelAvailable( float fuel );
		virtual float getFuelAvailable();
		virtual float getFuelAvailablePercent();
		virtual bool hasFuel();
		// End -- Reimplement FuelConsumptionModelBase methods

		ConfigParams& getConfigParams();

	};

}
