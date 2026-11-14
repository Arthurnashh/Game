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
#include "RTD/VehicleBase.h"
#include "Physics/smGraph2D.h"

namespace RTD
{
	/**
		Creates an air drag force which always acts in a direction which is opposite to the velocity and
		proportional to square (or cube) of the magnitude of the speed.

		This force generator does not take coefficient of drag and cross sectional area of the object
		as single and separate parameters, instead it contains a curve.	This curve stores products of 
		drag coefficient and crossectional area for different angles of incidence of relative
		wind. The angle mentioned is the angle between the relative wind vector's projection on XY plane
		and Y axis.

		X axis of the curve contains the incidence angle in degrees. It should cover the
		angles from -180 to 180.  Negative values mean relative wind from left hemisphere
		and positive values mean from right hemisphere.

		Y axis is Cd(unitless) * Area(m^2)
		Note: since -180 is the same as 180 the Y value for them should be equal.

		FGairDrag200 takes the atmospheric wind and air density into account.

	*/
	class RTDYNAMICS_EXPORT FGAirDrag200 : public ForceGenerator
	{
	public:

		/**
			Configuration parameters.
		*/		
		class RTDYNAMICS_EXPORT ConfigParams
		{
			public:
				/** Constructor of the class. */
				ConfigParams();

				/** If set to true a resistive torque for rotational speed is generated. See m_CdRot. */
				bool				m_rotationalDamping ;

				/** The drag coefficient for rotational motion. The torque that resists rotational motion
					will be proportinal to square of rotational speed and this variable. */
				smReal				m_CdRot ;				

				/** If set to true air drag will be proportional to the cube of the speed instead of square. */
				bool				m_cubic;

				/** This is a curve of Cd(unitless) * Area(m^2) indexed by the angle of incidence. */
				smGraph2D			m_CdTimesAreaCurve;

				virtual bool	readParamStr( const std::string& parameterString , std::string& error );
		};

		/**
			Constructor of the class.

			\param	name		:	Name of the instance.
			\param	vehicle		:	The vehicle on which the drag will act.
			\param	parameters	:	Configuration parameters.
		*/
		FGAirDrag200(	const std::string& name ,
						VehicleBase* vehicle,
						const ConfigParams& parameters  ) ;


		/**
			Returns the Cd*Area in given angle of incidence. Angle is in degrees
			and can be between -180 and 180 inclusive. If the curve does not contain
			the information or angleOfIncidence is outside the boundaries 0.0 is returned.
		*/
		smReal	getGetCdTimesArea( smReal angleOfIncidence );

		/** Updates the force generator. */
		virtual void update( smTime /* timestep */ ) ;

	protected:
		OutputVariableVec3r				o_generatedForce ;
		OutputVariableVec3r				o_generatedTorque ;

		VehicleBase*					m_vehicle;

		ConfigParams					m_configParams;

	};

}
