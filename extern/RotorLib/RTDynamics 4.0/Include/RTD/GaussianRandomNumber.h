/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/ParamDb.h"

namespace RTD
{
	class RTDYNAMICS_EXPORT GaussianRandomNumber : public SimulationVariableUser
	{
	public:
		GaussianRandomNumber(const std::string& name, int randomSeed , SimulationVariableUser* parent);

		void reset();
		double generate();

	protected:
		StateVariableDouble		V1;
		StateVariableDouble		V2;
		StateVariableDouble		S;
		StateVariableInt		phase;
		StateVariableInt		s_random;
	};
}
