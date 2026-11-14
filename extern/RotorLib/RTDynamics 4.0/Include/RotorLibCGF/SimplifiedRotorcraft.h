/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/MBDVehicle.h"
#include "RTD/GAVDynamics.h"
#include "RTD/FGAirDrag.h"
#include "RTD/FGSimpleThrust.h"
#include "RTD/TraitGroundContact.h"


namespace RotorLibCGF
{
	class SimplifiedRotorcraftFuelModel;

	/**
	*	The SimplifiedRotorcraft implements a generic CGF rotorcraft flight dynamics model and serves
	*	as the base class for all helicopter models that are controllable by a RotorLib CGF bot.
	*   SimplifiedRotorcraft has a simplified dynamics model to enable higher performance by
	*	reducing dynamics computations and control algorithm complexity.
	*
	*	SimplifiedRotorcraft can be configured using an XML based configuration file. Flight characteristics 
	*	of different helicopters can be modeled with different XML files.
	*
	**	@see Flight Dynamics Model section of the RotorLib CGF chapter in the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT SimplifiedRotorcraft : public RTD::MBDVehicle
	{
	public:
		/**
			Configuration parameters of the SimplifiedRotorcraft.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::VehicleBase::ConfigParams
		{
		public:
			/** Constructor of the class. */
			ConfigParams();


			/** Drag coefficient for translational motion. */
			smReal				m_CdTrans ;

			/**
				Maximum speed (in m/s) of this helicopter in level flight. It can be due to
				performance limitations or flight safety (i.e. to prevent retreating
				blade stall).

				The bot which controls this rotorcraft will try not to exceed this speed.
			*/
			smReal				m_maxLevelSpeed;

			/** Maximum lift that can be produced by the main rotor(s). In Newtons. */
			smReal				m_maxMainRotorLift;

			/** Radius of the main rotor in meters.*/
			smReal				m_mainRotorRadius;

			/** Maximum lift that can be produced by the tail rotor. In Newtons. */
			smReal				m_maxTailRotorLift;

			/**
				Position of the tail rotor relative to helicopter's center of gravity. It is assumed
				that tail rotor is on the Y axis (hence only a single real number is needed) and
				behind the center of gravity (so m_tailRotorPosition should be always a negative
				number ). In meters.
			*/
			smReal				m_tailRotorPosition;


			/**
				This parameter is used to adjust the effect of the longitudinal cyclic. The higher
				is the value the more powerful will be the pitching moment for a given cyclic setting.
			*/
			smReal				m_pitchControlGain;

			/**
				This parameter is used to adjust the effect of the lateral cyclic. The higher
				is the value the more powerful will be the rolling moment for a given cyclic setting.
			*/
			smReal				m_rollControlGain;

			/**
				The force output of the main rotor is deliberatly perturbed by a noise function
				to account for the effect of chaotic air flow. This prevents a too perfect and
				smooth flight. This parameter is used to adjust the strength of noise.

			*/
			smReal				m_mainRotorNoiseStrength;

			/**
				The force output of the tail rotor is deliberatly perturbed by a noise function
				to account for the effect of chaotic air flow. This prevents a too perfect and
				smooth flight. This parameter is used to adjust the strength of noise.
			*/
			smReal				m_tailRotorNoiseStrength;

			/** This parameter defines how often the noise force will change. Smaller values result in
				a calmer flight because the noise will not change often and the control system will need
				to make less corrections.

				Note that setting it to 0 will not disable the noise force. It will just make it constant.
				This parameter is in seconds.
			*/
			smReal				m_noiseChangeTime;

			/** Available fuel in kg. */
			smReal				m_fuelAvailable;
			
            /** Fuel consumption rate in kg/sec at maximum collective displacement. Actual fuel consumption rate is computed as m_fuelConsDueToCollPerSecond * normalized_collective_position . */
			smReal				m_fuelConsDueToCollPerSecond;

			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error \e false
				is returned	and \e error will contain the reason.

				SimplifiedRotorcraft::ConfigParams::load() method uses this method to parse the parameter file.
			*/
			virtual bool	readParamStr( const std::string& parameterString , std::string& error );
		};


		/**
			Constructor of the class.
			\param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.

			\param name			A unique name for the helicopter. If there is another vehicle with the same name
								within the same world, the newly created helicopter will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

			\param params	:	\e params contains the configuration parameters of the helicopter. The \e params
								pointer will \e not be deleted automatically.
		*/
		SimplifiedRotorcraft( RTD::World* world , const std::string& name, ConfigParams* params );

		/**
			Constructor of the class.
			\param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.

			\param name			A unique name for the helicopter. If there is another vehicle with the same name
								within the same world, the newly created helicopter will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

			\param parameterFile :	Name of an XML based configuration file. When this constructor is called, SimplifiedRotorcraft
                                    will be initialized with this configuration file. If there is a problem in reading the file,
									helicopter will be disabled. The format of this configuration file is explained in the
									RotorLib CGF section of the Programmer's Guide. 

			@see isVehicleCorrectlyConfigured()

		*/
		SimplifiedRotorcraft( RTD::World* world , const std::string& name, const std::string& parameterFile  );

		virtual void	preUpdate( smTime stepTime ) ;
		virtual void	postUpdate( smTime stepTime ) ;

		virtual void	reset();


		/**	Cyclic control input. Valid values are in [-1,1] range.
			Positive \e longitudinal pitches the nose down.
			Positive \e lateral rotates the helicopter clockwise (from pilot's point of view).
		*/
		void setCtrlCyclic		( smReal lateral, smReal longitudinal );

		/** Lateral cyclic input.
			Positive \e lateral rotates the helicopter clockwise (from pilot's point of view).
		*/
		void setCtrlCyclicLat	( smReal lateral );

		/** Longitudinal cyclic input.
			Positive \e longitudinal pitches the nose down.
		*/
		void setCtrlCyclicLon	( smReal longitudinal );

		/** Anti-torque pedals.
			Valid range is [-1,1]. Positive values rotate towards right (from pilot's point of view).
		 */
		void setCtrlPedal		( smReal pedal );

		/**
			Collective input. Valid range is [0,1].
		*/
		void setCtrlCollective	( smReal throttle );

		/** Returns current lateral cyclic input. */
		smReal getCtrlCyclicLat	( );

		/** Returns current Longitudinal cyclic input. */
		smReal getCtrlCyclicLon	( );

		/** Returns current anti-torque pedal input. */
		smReal getCtrlPedal		( );

		/** Returns current collective input. */
		smReal getCtrlCollective		( );

		/** Returns the configuration parameters of the helicopter. */
		const ConfigParams*	getConfigParams() const		{	return m_configParams;	}

		/** Returns the configuration parameters of the helicopter. */
		ConfigParams*		getConfigParams()			{	return m_configParams;	}

	protected:
		/**
			The create method is called by the constructor of the SimplifiedRotorcraft.
		*/
		virtual	void		create();

		/**
		*   Destructor of the class. This method is intentionally protected, use
		*   remove() method instead.
		*/
		virtual ~SimplifiedRotorcraft();
	
		RTD::StateVariableReal	s_noiseTimer;
		ConfigParams*	m_configParams;

		std::vector<RTD::FGSimpleThrust*>	m_mainRotorForces;
		std::vector<RTD::FGSimpleThrust*>	m_tailRotorForces;


		RTD::FGAirDrag*				m_airDrag ;

		// For control2 methods
		RTD::StateVariableReal	s_collective ;
		RTD::StateVariableReal	s_cyclicX, s_cyclicY ;
		RTD::StateVariableReal	s_pedals;

		RTD::StateVariableInt	s_random;

		int getRand();
	};
}