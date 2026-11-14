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

#include "RTD/RotorModelBase.h"
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
		- from within the code by creating an instance of the GenHeli600::ConfigParams class and
		  setting the values of the parameters or
		- providing an XML file, which contains the parameters, to the constructor. See Example-GenHeli600-DesktopSim.xml
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
	class RTDYNAMICS_EXPORT GenHeli600	: public RTD::MBDVehicle										
	{
	public:
		/**
		*	Configuration parameters of the helicopter.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::VehicleBase::ConfigParams
		{
			friend class GenHeli600;
		public:
			/** Constructor of the class.	*/
			ConfigParams();

			/** Configuration parameters of fuselage model. */
			RTD::FGHeliFuselage100::ConfigParams		m_fuselageParams;

			/**
				@name Stabilizers

				Following parameters are used to configure the rotors.			
			*/
			//@{
			std::vector<std::string>							m_rotorModelNames;
			std::vector<RTD::ModelComponentBase::ConfigParams*>	m_rotorModelParameters;
			//@}

			/**
				@name Stabilizers

				Following parameters are used to configure the stabilizers.

				@see RotorLibFDM::FGHelicopterStabilizerWing
			*/
			//@{
			std::vector<std::string>								m_stabilizerNames;
			std::vector<FGHelicopterStabilizerWing::ConfigParams*>	m_stabilizerParams;
			//@}


			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.

				GenHeli600::ConfigParams::load() method uses this method to parse the parameter file.
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
		GenHeli600(	RTD::World* world,
					const std::string& name,
					ConfigParams* params  );

		/**
			Constructor of the class.
			\param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.                                

			\param name			A unique name for the helicopter. If there is another vehicle with the same name
								within the same world, the newly created helicopter will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

			\param parameterFile :	Name of an XML based configuration file. When this constructor is called, GenHeli600
                                    will be initialized with this configuration file. If there is a problem in reading the file,
									helicopter will be disabled. The format of this configuration file is explained in the
									RotorLib FDM section of the Programmer's Guide. 

		*/
		GenHeli600( RTD::World* world , const std::string& name, const std::string& parameterFile );

		virtual void	preUpdate( smTime stepTime ) ;
		virtual void	postUpdate( smTime stepTime ) ;

		/** Resets the class. */
		virtual void	reset() ;

		/** Returns the main rotor of the helicopter. */
		RTD::RotorModelBase*					getMainRotor()				{	return m_mainRotor;	}

		/** Returns the main rotor of the helicopter. */
		const RTD::RotorModelBase*				getMainRotor() const		{	return m_mainRotor;	}

		/** Returns the tail rotor of the helicopter. */
		RTD::RotorModelBase*					getTailRotor()				{	return m_tailRotor;	}

		/** Returns the tail rotor of the helicopter. */
		const RTD::RotorModelBase*				getTailRotor() const		{	return m_tailRotor;	}

		/**
            Returns the number of stabilizers attached to the helicopter.
        */
		unsigned int							getNumStabilizers();

        /**
            Returns the stabilizer with given index. If the index is out of range, NULL is returned.
        */
		FGHelicopterStabilizerWing*				getStabilizer(unsigned int index );

        /**
            Returns the stabilizer with given index. If the index is out of range, NULL is returned.
        */
		const FGHelicopterStabilizerWing*		getStabilizer(unsigned int index ) const;


		/** Returns the drag on helicopter. */
		RTD::FGHeliFuselage100*				getFuselage()			{	return m_fuselage;	}

		
		/** Returns the configuration parameters of the airplane. */
		virtual const ConfigParams* getConfigParams() const     {   return m_configParams;  }

		/** Returns the configuration parameters of the airplane. */
		virtual ConfigParams*       getConfigParams()           {   return m_configParams;  }

        /**
            An engine callback is called periodically during the trim process which may take long time to finish.
            It provides access to internal variables of the helicopter model and current error of the trim solution.

            A trim callback can be used to print debugging information, handle I/O during the long trim process or
            abort the trim operation.
        */
		class TrimCallback 
		{
		public:
            /**
                This method is called periodically during the trim process. If the return value is false, trim process is aborted.

                \param error Current error level of the trim process. Once this number falls below the error threshold, the trim process ends.
                \param trimHeli Helicopter instance which is used during trim process. As the trim process progresses, the internal variables of this helicopter
                instance is modified and converge to the final solution. This pointer is valid during the trim process.
            */
			virtual bool update( double error , GenHeli600* trimHeli ) = 0;
		};


        /**
            This enum provides a categorization of commonly used engine operating regimes. Each of the power settings
            is associated with a power and torque rating.
        */
		enum EnginePowerSetting
		{
			EMERGENCY = 0,
				TAKE_OFF,
				MAX_CONTINUOUS
		};

		/**
            Returns the torque and power limitations at the given engine power setting.

            \param eps  Engine power setting for which the limits must be reported.
            \param torqueLimit  Torque limit in N*m. This includes both drive train and engine torque limitations.
            \param powerAtLimitTorque  Total power available at limit torque in Watts.
        */
		virtual void				getTorqueLimit( const EnginePowerSetting& eps , smReal& torqueLimit , smReal& powerAtLimitTorque ) const;


		/**
            This method is used to find the maximum climb rate of the helicopter at a given flight state and engine rating.
            It works by finding trim points at given speed, altitude and engine power. Engine power degree of freedom is
            controlled using collective.

            \param speedKts Ground speed in knots.
            \param altitudeFeet Altitude in feet.
            \param eps Engine power setting for which the maximum climb rate must be found.
            \param errorThreshold This parameter can be used to increase / decrease the fidelity level of the solution. Smaller error thresholds 
            result in higher fidelity. Fidelity level affects the computation time.
            \param startFromCurrentState If this argument is true, the search for the trim solution will begin from the current state of the helicopter
            ,i.e. helicopter will not be reset before the process.
            \param printInfo If set to true, debugging information regarding the trimming process will be printed to the console.
            \param tc Trim callback which is called periodically during the trim process. This argument can be NULL.

        */
		virtual smReal				findMaxClimbRateFtMin(	 smReal speedKts, smReal altitudeFeet,
															 EnginePowerSetting eps,
		 													 smDouble errorThreshold ,
															 bool startFromCurrentState,
															 bool printInfo,
															 TrimCallback* tc );

		/**
            This method is used to find the maximum level flight speed of the helicopter at a given altitude and engine rating.
            It works by finding trim points at given altitude and engine power. Engine power degree of freedom is
            controlled using collective.

            \param altitudeFeet Altitude in feet.
            \param eps Engine power setting for which the maximum speed must be found.
            \param errorThreshold This parameter can be used to increase / decrease the fidelity level of the solution. Smaller error thresholds 
            result in higher fidelity. Fidelity level affects the computation time.
            \param startFromCurrentState If this argument is true, the search for the trim solution will begin from the current state of the helicopter
            ,i.e. helicopter will not be reset before the process.
            \param printInfo If set to true, debugging information regarding the trimming process will be printed to the console.
            \param tc Trim callback which is called periodically during the trim process. This argument can be NULL.

        */
		virtual smReal				findMaxSpeedKts(		 smReal altitudeFeet,
															 EnginePowerSetting eps,
		 													 smDouble errorThreshold ,
															 bool startFromCurrentState,
															 bool printInfo,
															 TrimCallback* tc );

        /**
            This method is used to find trim solutions for a user defined flight state at a given position. Trim solution is always found
            with zero heading (a minor offset is possible if the solution requires side slip),

            \param trimPosition World coordinate position in meters.
            \param trimVelocity Velocity in world coordinates. This vector is in meters.
            \param errorThreshold This parameter can be used to increase / decrease the fidelity level of the solution. Smaller error thresholds 
            result in higher fidelity. Fidelity level affects the computation time.
            \param startFromCurrentState If this argument is true, the search for the trim solution will begin from the current state of the helicopter
            ,i.e. helicopter will not be reset before the process.
            \param printInfo If set to true, debugging information regarding the trimming process will be printed to the console.
            \param tc Trim callback which is called periodically during the trim process. This argument can be NULL.

        */
		virtual void				trim(	const smVec3d& trimPosition , const smVec3r& trimVelocity,
											smDouble errorThreshold = 0.0001,					
											bool startFromCurrentState = false,
											bool printInfo = false,
											TrimCallback* tc = NULL );

	protected:
		class TrimFinder;

		std::vector<FGHelicopterStabilizerWing*>	m_stabilizers;


		/** Destructor of the class. The destructor is intentionally declared as a protected destructor. Use remove() method to delete an instance of this class. */
		virtual ~GenHeli600( );

		void						createLoggingPrinter() ;
		virtual	void				create();

		protected:
		ConfigParams*				m_configParams;
		RTD::RotorModelBase*		m_mainRotor ;
		RTD::RotorModelBase*		m_tailRotor ;

		RTD::FGHeliFuselage100*		m_fuselage ;


		virtual	void			updateDataLogger();		
		
	};
}

/**
*	\namespace RotorLibFDM
*
*	The RotorLibFDM namespace includes high fidelity flight dynamics model (FDM) for helicopter simulation.
*/

