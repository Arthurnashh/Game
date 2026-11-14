/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/RigidBody.h"
#include "RTD/RigidBodyState.h"
#include "RTD/ForceGenerator.h"

namespace RTD
{
	/** ForceAndTorqueAccumulator can be used to compute the total sum of forces 
	*	and torques on an aircraft.
	*/
	class RTDYNAMICS_EXPORT ForceAndTorqueAccumulator
	{
	public:
		// Updates the state.c_sumForces and state.c_sumTorques fields
		// of a rigid body.
		static void calculate(	smTime timestep,
								smManager<ForceGenerator>& forceGens, 
								RigidBodyStateBase& state,
								const RigidBody* body = NULL ) ;
	};

	/** DynamicsIFace serves as a base class for dynamics engines in RTD. */
	class VehicleBase ;
	class RTDYNAMICS_EXPORT DynamicsEngineBase : public Base
	{
	protected:
		VehicleBase* m_ac ;

	public:
		DynamicsEngineBase( VehicleBase* ac ) : m_ac(ac)
		{};

		virtual void update(smTime timestep) = 0 ; 
	};
}
