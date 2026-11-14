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


namespace RTD
{
	class VehicleBase;

	/**
	*	Creates an air drag force which always acts in a direction which is opposite to the velocity and
	*	proportional to square (or cube) of the magnitude of the speed.
	*
	*	FGAirDrag is affected by air density (see setAirDensity() ) and wind.
	*
	*	The moment from this force generator allows rotation air drag which slows down the rigid body when it is
	*	rotating.
	*/
	class RTDYNAMICS_EXPORT FGAirDrag : public ForceGenerator
	{
	public:
		/**
			Constructor of the class.

			\param	name	:	Name of the instance.
			\param	aircraft:	The aircraft on which the drag will act.
			\param	cubic	:	If set to true air drag is proportional to the cube of the velocity instead of square.
		*/
		FGAirDrag( const std::string& name , VehicleBase* aircraft , bool cubic=false ) ;

		/** Sets/gets the coefficient of drag.	*/
		//@{
		void    setCd( const smCoefPos& Cd );
		smReal	getCd() const					{	return m_Cd;	}
		//@}

		/** Sets/gets the coefficient of rotational drag.	*/
		//@{
		void    setRotationalDragCoeff( const smCoefPos& rotDrag ) ;
		smReal	getRotationalDragCoeff() const	{	return m_CdRot;	}
		//@}

		/** Sets/gets the characteristic cross sectional area of the object.*/
		//@{
		void	setCrossectionalArea( const smCoefPos& area );
		smReal	getCrossectionalArea() const	{	return m_crossectionalArea;	}
		//@}

		/** Sets/gets the air density.	*/
		//@{
		void	setAirDensity( const smCoefPos& d );
		smReal	getAirDensity() const			{	return i_airDensity.getValue();	}
		//@}

		/** Updates the force generator. */
		virtual void update( smTime /* timestep */ ) ;

	public:
		/** Keeps reference to owning rigid body object.	*/
		VehicleBase*					m_aircraft;
		
		/** If true drag is proportional to the cube of speed   */
		bool							m_cubic;
		
		smReal							m_crossectionalArea;

		bool							m_rotationalDamping ;

		/** Drag Coefficient.	*/
		smReal							m_Cd ;

		/** Rotational Damping Coefficient */
		smReal							m_CdRot ;

		InputVariableReal				i_airDensity;

		/* */
		OutputVariableVec3r				o_generatedForce ;

		/** Generated rotational damping torque	*/
		OutputVariableVec3r				o_generatedTorque ;






	};
}
