/*
(c) Copyright 2009, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "FixedWingLibCGF/SimplifiedAirplane.h"

namespace ArmamentLib
{

class RTDYNAMICS_EXPORT Bomb : public FixedWingLibCGF::SimplifiedAirplane 
{
public:
	Bomb(RTD::World* world , const std::string& name, const std::string& parameterFile  );

	virtual void preUpdate( smTime timestep );
};

}