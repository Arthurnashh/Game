/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/FGAirDrag.h"


namespace RTD
{

	/**
		There are multiple versions of air drag. FGAirDrag100 is the first
		version developed. FGAirDrag and FGAirDrag100 are equivalent to ensure
		backward compatibility. See FGAirDrag for details of air drag implementation.
	*/
	typedef	 RTD::FGAirDrag FGAirDrag100;



}
