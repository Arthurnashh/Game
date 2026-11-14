/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/ForceGenerator.h"
#include "Util/smTimeBasedPulse.h"
#include "RTD/VehicleBase.h"

namespace RTD
{
	/**
	*	Basically yet another force generator, but FGImpulse is momentary
	*	and will only generate force for one iteration.
	*
	*	You can use FGForceImpulse if you want to apply the impulse for a certain
	*	period of time (in seconds).
	*/
	class RTDYNAMICS_EXPORT FGImpulse : public ForceGenerator
	{
	public:
		FGImpulse( const std::string& name , VehicleBase* owner ) ;

		void	trigger( )
		{	c_triggered = 1.0f ; }

		virtual void update( smTime timestep ) ;

	public:
		/** Maximum force which can be generated.	*/
		smForce         m_maxThrust  ;

		/** Current trigger.	*/
		smBool		    c_triggered ;

		VehicleBase*	m_vehicle;
	};
}
