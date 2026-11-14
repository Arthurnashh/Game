/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once




#include "RTD/RTD.h"
#include "RTD/MBDVehicle.h"

#include "RTD/FGRotorMomentumTheory.h"
#include "RotorLibFDM/FGHeliFuselage100.h"
#include "RTD/FGTorque.h"
#include "RTD/FGGroundContactBase.h"
#include "RTD/EngineModelBase.h"
#include "RTD/Instruments.h"

#include "RotorLibFDM/FGHelicopterStabilizerWing.h"


namespace RotorLibFDM
{
	

	/** 
		This class implements a generic helicopter flight dynamics model. To model a specific helicopter
		type suitable configuration parameters should be provided to the model. These parameters can
		be provided
		- from within the code by creating an instance of the GenHeli500::ConfigParams class and
		  setting the values of the parameters or
		- providing an XML file, which contains the parameters, to the constructor. See Example-GenHeli500-DesktopSim.xml
		  file for an example.

		Main features of the model are:
		- One main rotor.
		- One tail rotor.
		- Extensible engine model architecture.
		- A drive train model which computes the distribution of the engine torque among main and tail rotors.
		- Extensible flight control system architecture.
		- Reconfigurable ground reaction model (landing gears, skids).
		- Instrument model.

	*/
	class RTDYNAMICS_EXPORT GenHeli500	: public RTD::MBDVehicle										
	{
	public:
		/**
		*	Configuration parameters of the helicopter.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::VehicleBase::ConfigParams
		{
			friend class GenHeli500;
		public:

			/** Constructor of the class.	*/
			ConfigParams();

			/** Configuration parameters of fuselage model. */
			RTD::FGHeliFuselage100::ConfigParams		m_fuselageParams;

			/**
				Configuration parameters of the main rotor.
				@see RTD::FGRotorMomentumTheory
				@see RTD::FGRotorMomentumTheory::ConfigParams
			*/
			RTD::FGRotorMomentumTheory::ConfigParams	m_mainRotorParams;

			/**
				Configuration parameters of the tail rotor.
				@see RTD::FGRotorMomentumTheory
				@see RTD::FGRotorMomentumTheory::ConfigParams
			*/
			RTD::FGRotorMomentumTheory::ConfigParams	m_tailRotorParams;

			/**
				@name Stabilizers

				Following parameters are used to configure the stabilizers.

				@see RotorLibFDM::FGHelicopterStabilizerWing
			*/
			//@{
			FGHelicopterStabilizerWing::ConfigParams*	m_horizontalStabilizerRightParams;
			FGHelicopterStabilizerWing::ConfigParams*	m_horizontalStabilizerLeftParams;
			FGHelicopterStabilizerWing::ConfigParams*	m_verticalStabilizerParams;
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
			\param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.                                

			\param name			A unique name for the helicopter. If there is another vehicle with the same name
								within the same world, the newly created helicopter will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

			\param params	:	\e params stores the configuration parameters of the helicopter. The \e params
								pointer will \e not be deleted automatically.
		*/
		GenHeli500(	RTD::World* world,
					const std::string& name,
					ConfigParams* params  );

		/**
			Constructor of the class.
			\param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.                                

			\param name			A unique name for the helicopter. If there is another vehicle with the same name
								within the same world, the newly created helicopter will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

			\param parameterFile :	Name of an XML based configuration file. When this constructor is called, GenHeli500
                                    will be initialized with this configuration file. If there is a problem in reading the file,
									helicopter will be disabled. The format of this configuration file is explained in the
									RotorLib FDM section of the Programmer's Guide. 

		*/
		GenHeli500( RTD::World* world , const std::string& name, const std::string& parameterFile );

		virtual void	preUpdate( smTime stepTime ) ;
		virtual void	postUpdate( smTime stepTime ) ;

		/** Resets the class. */
		virtual void	reset() ;

		/** Returns the main rotor of the helicopter. */
		RTD::FGRotorMomentumTheory*				getMainRotor()				{	return m_mainRotor;	}

		/** Returns the main rotor of the helicopter. */
		const RTD::FGRotorMomentumTheory*		getMainRotor() const		{	return m_mainRotor;	}

		/** Returns the tail rotor of the helicopter. */
		RTD::FGRotorMomentumTheory*				getTailRotor()				{	return m_tailRotor;	}

		/** Returns the tail rotor of the helicopter. */
		const RTD::FGRotorMomentumTheory*		getTailRotor() const		{	return m_tailRotor;	}

		/** Returns the right horizontal stabilizer. */
		FGHelicopterStabilizerWing*				getHorizontalStabilizerRight()					{	return m_horizontalStabilizerRight;	}

		/** Returns the right horizontal stabilizer. */
		const FGHelicopterStabilizerWing*		getHorizontalStabilizerRight() const			{	return m_horizontalStabilizerRight;	}

		/** Returns the right horizontal stabilizer. */
		FGHelicopterStabilizerWing*				getHorizontalStabilizerLeft()					{	return m_horizontalStabilizerLeft;	}

		/** Returns the right horizontal stabilizer. */
		const FGHelicopterStabilizerWing*		getHorizontalStabilizerLeft() const				{	return m_horizontalStabilizerLeft;	}

		/** Returns the right horizontal stabilizer. */
		FGHelicopterStabilizerWing*				getVerticalStabilizer()							{	return m_verticalStabilizer;	}

		/** Returns the right horizontal stabilizer. */
		const FGHelicopterStabilizerWing*		getVerticalStabilizer() const					{	return m_verticalStabilizer;	}


		/** Returns the drag on helicopter. */
		RTD::FGHeliFuselage100*				getFuselage()			{	return m_fuselage;	}

		
		/** Returns the configuration parameters of the airplane. */
		virtual const ConfigParams* getConfigParams() const     {   return m_configParams;  }

		/** Returns the configuration parameters of the airplane. */
		virtual ConfigParams*       getConfigParams()           {   return m_configParams;  }


		/** Experimental feature.
		 */
		class TrimCallback 
		{
		public:
			virtual bool update( double error , GenHeli500* trimHeli ) = 0;
		};

		/** Experimental feature.
		 */
		virtual void				trim(	const smVec3d& trimPosition , const smVec3r& trimVelocity,
											smDouble errorThreshold = 0.0001,					
											bool startFromCurrentState = false,
											bool printInfo = false,
											TrimCallback* tc = NULL );

	protected:
		class TrimFinder;

		FGHelicopterStabilizerWing*			m_horizontalStabilizerRight;
		FGHelicopterStabilizerWing*			m_horizontalStabilizerLeft;
		FGHelicopterStabilizerWing*			m_verticalStabilizer;


		/** Destructor of the class. The destructor is intentionally declared as a protected destructor. Use remove() method to delete an instance of this class. */
		virtual ~GenHeli500( );

		// Computes how the engine torque is shared among the main and tail rotors
		// and returns the torques delivered to main and tail rotors.
		// Positive torques, angular velocities and angular accelerations denote
		// rotations from X axis towards Y axis in RotorLib coordinate system.
		// Inputs:
		// ---------------------------------------------------------------
		// Te	: Torque delivered by the engine (N*m)
		// Tme	: External torque (because of aerodynamic and frictional forces) on the main rotor (N*m)
		// Tte	: External torque (because of aerodynamic and frictional forces) on the tail rotor (N*m)
		// Km	: Main rotor gear ratio ( main rotor RPM = engine RPM / Km ) (unitless)
		// Kt	: Tail rotor gear ratio ( tail rotor RPM = engine RPM / Kt ) (unitless)
		// Im	: Main rotor moment of inertia (kg*m^2)
		// It	: Tail rotor moment of inertia (kg*m^2)
		//
		// Outputs
		// ---------------------------------------------------------------
		// dWe	: Engine output shaft angular acceleration ( rad / sec^2 )
		// dWm	: Main rotor angular acceleration ( rad / sec^2 )
		// dWt	: Tail rotor angular acceleration ( rad / sec^2 )
		// Tdm	: Torque delivered to main rotor by the engine
		// Tdt	: Torque delivered to tail rotor by the engine
		virtual void				updateDriveTrain(	smReal Te ,
														smReal Tme , smReal Tte ,
														smReal Im , smReal It ,
														smReal Km,smReal Kt,
														smReal& dWe,
														smReal& dWm,
														smReal& dWt,
														smReal& Tdm,
														smReal& Tdt );

		void						createLoggingPrinter() ;
		virtual	void				create();

		protected:
		ConfigParams*				m_configParams;
		RTD::FGRotorMomentumTheory*	m_mainRotor ;
		RTD::FGRotorMomentumTheory*	m_tailRotor ;

		RTD::FGHeliFuselage100*		m_fuselage ;


		virtual	void			updateDataLogger();		
		
	};
}

/**
*	\namespace RotorLibFDM
*
*	The RotorLibFDM namespace includes high fidelity flight dynamics model (FDM) for helicopter simulation.
*/

