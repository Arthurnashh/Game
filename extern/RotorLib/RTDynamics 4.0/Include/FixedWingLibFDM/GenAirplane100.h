/*
(c) Copyright 2006, RTDynamics
All Rights Reserved.

Product:		FixedWingLib
Subsystem:		FixedWingLibFDM

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "FixedWingLibFDM/Common.h"
#include "RTD/MBDVehicle.h"
#include "RTD/FGWing.h"
#include "RTD/FGTorque.h"
#include "RTD/FGSimpleThrust.h"
#include "RTD/FGWing.h"
#include "RTD/FGAirDrag100.h"
#include "RTD/FGAirDrag200.h"
#include "RTD/ControlSystemComponentBase.h"

namespace RTD
{
	class GAVDynamics ;
}

namespace FixedWingLibFDM
{
	/**
		This class implements a generic airplane flight dynamics model. To model a specific airplane
		type suitable configuration parameters should be provided to the model. These parameters can
		be provided  from within the code by creating an instance of the GenAirplane100::ConfigParams
		class and setting the values of the parameters.

		Main features of the model are:
		- Separately configurable main wings, horizontal and vertical stabilizers.
		- Extensible engine model architecture.
		- Extensible flight control system architecture.
		- Reconfigurable ground reaction model (landing gears).
		- Fuselage drag model.

	*/

	class RTDYNAMICS_EXPORT GenAirplane100 : public RTD::MBDVehicle
	{
	public:
		/**
		*	Configuration parameters of the airplane.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::VehicleBase::ConfigParams
		{
			friend class GenAirplane100;
		public:
			/** Constructor of the class.	*/
			ConfigParams();


			/**
				@name Wings & Stabilizers

				This model consists of two main wings, two tail wings and
				one vertical stabilizer.

				Following parameters are used to configure the wings and stabilizers.

				@see RTD::FGWing
			*/
			//@{
			RTD::FGWing::ConfigParams	m_leftWingParams;
			RTD::FGWing::ConfigParams	m_rightWingParams;
			RTD::FGWing::ConfigParams	m_leftTailWingParams;
			RTD::FGWing::ConfigParams	m_rightTailWingParams;
			RTD::FGWing::ConfigParams	m_verticalStabilizer1Params;
			//@}


			/**
				This is a curve which stores products of drag coefficient and crossectional area
				for different angles of incidence of relative wind. The angle mentioned is
				the angle between the relative wind vector's projection on XY plane and Y axis 
				(slipping angle or beta).

				X axis of the curve contains the incidence angle in degrees. It should cover the
				angles from -180 to 180. Negative values mean relative wind from left hemisphere
				and positive values mean from right hemisphere.

				Y axis is Cd(unitless) * Area(m^2)

				Note: since -180 is the same as 180 the Y value for them should be equal.
			*/
			smGraph2D			m_fuselageDragCurve;

			/** Drag coefficient of the spoilers. Spoilers create air drag only when they are deployed. */
			smCoefPos			m_spoilerDragCoeff;

			/** The reference cross sectional area of the spoilers to be used in air drag computation. In m^2. */
			smCoefPos			m_spoilerReferenceArea;

			/**
				The time needed to extend the spoilers fully. The time needed to
				retract the spoilers assumed to be the same. The value is in seconds.
			*/
			smCoefPos			m_spoilerExtensionTime;

			/**
				The time needed to change flap configuration to the next/previous one.
				The value is in seconds.
			*/
			smCoefPos			m_flapExtensionTime;


			/** Drag coefficient to damp rotational motion. */
			smCoefPos			m_rotationalDragCoeff ;

			/**
				@name Stability

				Usually aerodynamic moments acting on an airplane tends to increase its stability. These moments
				oppose rotational movement of the aircraft. For example, if roll rate is positive, angle of
				attack of the left and right wing is different due to rotational motion of the wings. This 
				creates a moment in the opposite direction of rotation.

				Standard aerdynamic configuration of the fuselage and wings usually provides some degree of stability,
				however, it is possible that the resulting model needs further tweaking. Following parameters are used to 
				adjust the stability in pitch, roll and yaw axes. Zero parameter value leaves the stability unchanged. Positive
				values increase and negative values decrease the stability.				
			*/
			//@{
			smCoefPos			m_aerodynamicRollStabilityCoeff;
			smCoefPos			m_aerodynamicPitchStabilityCoeff;
			smCoefPos			m_aerodynamicYawStabilityCoeff;
			//@}


			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.

				GenHeli500::ConfigParams::load() method uses this method to parse the parameter file.
			*/
			virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		protected:
			/** Copy constructor of the class. Deep copying is not supported and can result in
				hard to catch bugs if used. Because of this copy constructor is disabled. */
			ConfigParams( const ConfigParams& rhs );

			/** Assignment operator of the class. Deep copying is not supported and can result in
				hard to catch bugs if used. Because of this copy assignment operator is disabled. */
			virtual ConfigParams&	operator=(const ConfigParams& rhs );

			/** Do not allow a ConfigParams instance on the stack. ConfigParams should be a pointer
				always. */
			virtual ~ConfigParams();
		};

	public:
		/**
			Constructor of the class.
			\param world	:	An instance of World class which stores environmental information like wind,
								air pressure vs. altitude curve etc.

			\param name			A unique name for the aircraft. If there is another vehicle with the same name
								within the same world, the newly created aircraft will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

			\param params	:	\e params stores the configuration parameters of the airplane. The \e params
								pointer will \e not be deleted automatically.
		*/
		GenAirplane100(	RTD::World* world ,
							const std::string& name,
							ConfigParams* params  );

		/**
			Constructor of the class.
			\param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.                                

			\param name			A unique name for the aircraft. If there is another vehicle with the same name
								within the same world, the newly created aircraft will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

			\param parameterFile :	Name of an XML based configuration file. When this constructor is called, GenAirplane100
                                    will be initialized with this configuration file. If there is a problem in reading the file,
									airplane will be disabled. The format of this configuration file is explained in the
									FixedWingLib FDM section of the Programmer's Guide. 

		*/
		GenAirplane100( RTD::World* world , const std::string& name, const std::string& parameterFile );


		virtual void	preUpdate( smTime stepTime ) ;
		virtual void	postUpdate( smTime stepTime ) ;


		/** Resets the vehicle.	*/
		void			reset() ;

		/**
			Get configuration parameters of the airplane.
		*/
		const ConfigParams*	getConfigParams() const		{	return m_configParams;	}

		/**
			Get configuration parameters of the airplane.
		*/
		ConfigParams*	getConfigParams()				{	return m_configParams;	}


		/**
			@name Flap set/get

			Sets/gets active flap configuration. A value of 1.2 means, for example,
			flaps are between configuration 1 and 2 and the transition from 1 to
			2 is 20% (or 2 to 1 is 80%) completed.

			First configuration is 0.0
		*/
		//@{
		virtual void						setLeftFlapSetting( smReal status );
		smReal      						getLeftFlapSetting() const;
		virtual void						setRightFlapSetting( smReal status );
		smReal      						getRightFlapSetting() const;
		//@}

		/** Returns the number of different flap settings on this airplane. */
		int      							getNumFlapSettings() const;


		/**
			@name Spoiler set/get
			Set/get spoiler status. 0.0 means fully retracted, whereas 1.0
			means fully extended. Values outside this range is invalid.
		*/
		//@{
		virtual void						setSpoilerSetting( smReal extensionAmount );
		smReal								getSpoilerSetting() const;
		//@}

		/**
			@name Wing & stabilizer access
			Following methods are used to obtain pointers to wings and stabilizers.
		*/
		//@{
		RTD::FGWing*					getRightMainWing();
		const 	RTD::FGWing*			getRightMainWing() const;
		RTD::FGWing*					getLeftMainWing();
		const 	RTD::FGWing*			getLeftMainWing() const;
		RTD::FGWing*					getRightTailWing();
		const 	RTD::FGWing*			getRightTailWing() const;
		RTD::FGWing*					getLeftTailWing();
		const 	RTD::FGWing*			getLeftTailWing() const;
		RTD::FGWing*					getVerticalTailWing();
		const RTD::FGWing*			getVerticalTailWing() const;
		//@}

	protected:
		ConfigParams*			m_configParams;


		RTD::FGWing*			m_rightWing;
		RTD::FGWing*			m_leftWing;
		RTD::FGWing*			m_rightTailWing;
		RTD::FGWing*			m_leftTailWing;
		RTD::FGWing*			m_verticalTailWing;

		RTD::FGTorque*			m_aerodynamicRollStability;


		RTD::FGAirDrag100*		m_spoilerDrag;
		RTD::FGAirDrag200*		m_fuselageAirDrag ;

		RTD::StateVariableReal		s_spoilerExtensionAmount;
		RTD::StateVariableReal		s_leftFlapStatus;
		RTD::StateVariableReal		s_rightFlapStatus;

		unsigned int			m_flapCount;

		/**
			Destructor of the class. The destructor is intentionally declared as a protected destructor. Use remove() method to delete an instance of this class.
		*/
		virtual ~GenAirplane100( );

		virtual	void			create();

	};
}