/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/DynamicsEngineBase.h"

namespace RTD
{
	class RigidBodyObject;

	/**
	*   Generic Airborne Vehicle Dynamics Model. Deprecated, please don't use
	*   this class any more.
	*/
	class RTDYNAMICS_EXPORT GAVDynamics : public DynamicsEngineBase
	{
	public:
		public:
			GAVDynamics( RigidBodyObject* ac ) ;

		virtual void update(smTime timestep ) ;
	};
}
