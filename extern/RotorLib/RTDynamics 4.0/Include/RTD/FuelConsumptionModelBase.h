/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

namespace RTD
{
	class RTDYNAMICS_EXPORT FuelConsumptionModelBase
	{
	public:
		FuelConsumptionModelBase()
		{};

		virtual void reset() {} ; 
		virtual void update(smTime timestep) = 0 ; 
		virtual float getFuelConsumption() = 0;
		virtual void setFuelAvailable( float fuel ) = 0 ;
		virtual float getFuelAvailable() = 0 ;
		virtual bool hasFuel() = 0 ;
	};

}
