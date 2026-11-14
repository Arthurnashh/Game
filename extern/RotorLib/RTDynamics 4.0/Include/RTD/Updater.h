/*
(c) Copyright 2007, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Base.h"
#include "RTD/VehicleBase.h"

namespace RTD
{
	/**
	*	Updater class can be used to manage the simulation for an aircraft. It manages 
	*	the simulation step time and makes sure that the simulation quality is always 
	*	stable even when the delta time fluctuates.
	*
	*	Normally it is enough to construct an Updater object and call the update
	*	method.
	*/
	class RTDYNAMICS_EXPORT Updater
	{
	public:
		/** Create an Updater with the given aircraft.	*/
		Updater( RTD::VehicleBase* aircraft  ) ;

		/**
		*	In this version of the constructor init() is called with NULL arguments.
		*/
		Updater( ) ;

		/** Initialize this Updater to use the given aircraft. */
		virtual void	init( RTD::VehicleBase* aircraft ) ;

		/** @param deltaTime Delta time in seconds. */
		virtual void	update( smTime deltaTime ) ;

		/** Resets the aircraft as well as internal variables of this class.	*/
		virtual void	reset() ;

		/**	Using the set method, it is possible to change the internal update deltatime
		*	of this aircraft.
		*
		*	@param	fixedDeltaTime			Delta time in seconds. E.g. 0.010f for 100Hz.
		*
		*	@param	groundTouchFixedDeltaTime			Fixed delta time to be used if the aircraft touches the ground.
		*												This parameter will be effective only if the aircraft supports
		*												ground contact ( has RTD::VehicleGroundContact as parent)
		*
		*/
		virtual void	set( smReal fixedDeltaTime , smReal groundTouchFixedDeltaTime);


		/** Updates the aircraft. */
		virtual void updateVehicle( smReal deltaTime ) ;


		/** 
			Returns the fixed delta time as set by the user. 
			@see set()
		*/
		smReal	getFixedDeltaTime()	const						{	return m_fixedDeltaTime;	}

		/** 
			Returns the ground touch fixed delta time as set by the user. 
			@see set()
		*/
		smReal	getGroundTouchFixedDeltaTime()	const			{	return m_groundTouchFixedDeltaTime;	}


	protected:
		RTD::VehicleBase*		m_aircraft;
		smReal				m_timeAccumulator;

		smReal				m_fixedDeltaTime;		
		smReal				m_groundTouchFixedDeltaTime;
		
		int					m_vehicleUpdateCount ;

	};

}
