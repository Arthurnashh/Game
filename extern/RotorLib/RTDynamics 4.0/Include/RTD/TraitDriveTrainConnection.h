/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/Base.h"

namespace RTD
{


	/**
		Base class for mathematical models that can be connected to a drive train.
	*/
	class RTDYNAMICS_EXPORT TraitDriveTrainConnection 
	{
	public:

		virtual smDouble	getRPM() const	=	0;
		virtual void		setRPM( smDouble rpm )=	0;
		virtual smDouble	getExternalTorque() const = 0;
		virtual smDouble	getRotationalInertia() const = 0;

	};


}