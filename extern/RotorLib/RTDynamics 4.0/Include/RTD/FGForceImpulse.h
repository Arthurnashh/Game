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

namespace RTD
{
	/**
	*	ForceImpulse generates (only) a force on the body for a certain
	*	period of time defined by a pulse signal.
	*
	*	If the impulse needs to be applied only in one step, FGImpulse can
	*	be used.
	*/
	class RTDYNAMICS_EXPORT FGForceImpulse : public ForceGenerator
	{
	private:
		smTimeBasedPulse m_pulse ;
		smForceVec       m_forceVec ;

	public:
		FGForceImpulse( const std::string& name  , VehicleBase* owner )  ;

		virtual void  update( smTime timestep ) ;

		/**
		*	@param	force		the force vector
		*   @param	waitTime	specifies how long the pulse generator will wait
		*                   	for before starting the signal. This parameter is in seconds.
		*	@param	pulseTime	specifies how long the pulse will be generated. This parameter is in seconds.
		*	@param	repeat		specifies how many times the pulse will be repeated
		*/
		void setForce( const smForceVec& force, smReal waitTime = 0, smReal pulseTime=1, int repeat=1 ) ;
	};
}
