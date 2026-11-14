/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:		RTDynamics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/ForceGenerator.h"
#include <Physics/smGraph2D.h>
#include "RTD/ModelComponentBase.h"

namespace RTD
{
	class VehicleBase;
	class EngineModelBase;
	class FGGroundContactBase;
	/**
		DriveTrainModelBase class provides a base class for drive train model implementations. It provides the basic interface.

		@see GenericDriveTrain

	*/
	class RTDYNAMICS_EXPORT DriveTrainModelBase : public ModelComponentBase
	{
	public:

		/**
			Base class for defining parameters of drive train model implementations. A new drive train implementation
			must derive its own ConfigParams from this one.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public ModelComponentBase::ConfigParams
		{
		public:

            /** Constructor of the class. */
			ConfigParams();

            /** Destructor of the class. */
			virtual ~ConfigParams();
		};

		/** 
			Creates an instance of class.
			\param vehicle		:	The vehicle to which the drive train is attached.
			\param name			:   Name of the drive train instance.
			\param params		:	Configuration parameters of the drive train component.
		*/
		DriveTrainModelBase( RTD::VehicleBase* vehicle, const std::string& name, const ConfigParams* params  );


        /**
            Destructor of the class.
        */
		virtual ~DriveTrainModelBase();
		
        /** */
		virtual void			update( smTime stepTime )	=	0;

        /** */
		virtual void			reset();

        /** This method is used to set the gear state. */
		virtual	void			setGear( smReal gear )	{	s_gear = gear;	}

        /** Returns the gear state. */
		smReal					getGear() const			{	return s_gear;	}

        /** Set clutch state. Valid values are in [0..1] range. When the input is 0, clutch is engaged and transfers torque at maximum capacity.
            Clutch is disengaged when the input is 1. */
		virtual	void			setClutch( smReal clutch )	{	s_clutch = clutch;	}

        /** Returns clutch state. Valid values are in [0..1] range. When the input is 0, clutch is engaged and transfers torque at maximum capacity.
            Clutch is disengaged when the input is 1. */
		smReal					getClutch() const			{	return s_clutch;	}

        /** Returns the configuration parameters of this model. */
		virtual const	ConfigParams*   getConfigParams() const = 0;

        /** Returns the configuration parameters of this model. */
		virtual ConfigParams*		    getConfigParams()		= 0;

		/**
            Returns the drive train torque limit for maximum continuous operation rating.
        */
		virtual smReal					getMaxContinuousTorqueLimit() const;

		/**
            Returns the drive train torque limit for take off rating.
        */
		virtual smReal					getTakeoffTorqueLimit() const;

		/**
            Returns the drive train torque limit for emergency rating.
        */
		virtual smReal					getEmergencyTorqueLimit() const;

		/**
			This method returns the rate of revolution of the common engine shaft, i.e.
			the shaft to which all engines are connected.
		*/
		virtual smReal					getCommonEngineShaftRPM() const;

	protected:
		
		StateVariableReal		s_gear;
		StateVariableReal		s_clutch;

	};

    /**
        This class implements a generic drive train model.

        See "Generic Drive Train" section in the Programmer's Guide for more information.
    */
	class RTDYNAMICS_EXPORT GenericDriveTrain :	public DriveTrainModelBase
	{
	public:
        /**
            Configuration parameters of the model. 

            See "Generic Drive Train" section in the Programmer's Guide for more information.
        */
		class RTDYNAMICS_EXPORT ConfigParams : public DriveTrainModelBase::ConfigParams
		{
		public:
			ConfigParams();
			virtual ~ConfigParams();

			std::vector< smDouble	>	m_gearRatios;
			smDouble					m_reverseGearRatio;

			std::vector< unsigned int>	m_engineIndices;

			std::vector< std::string >	m_loadNames;
			std::vector< smReal >		m_loadGearRatios;


			std::vector< smReal	>		m_differential2GearRatios;
			std::vector< int>			m_differential2LoadIndex1;
			std::vector< int>			m_differential2LoadIndex2;

			smDouble					m_maxClutchTorque;
			bool						m_isClutchFreewheeling;

			smReal						m_maximumContinousTorqueLimit;
			smReal						m_takeoffTorqueLimit;
			smReal						m_emergencyTorqueLimit;

			smReal						m_nominalOilPressureBar;

			smReal						m_CESRPMAtNominalOilPressure;

			smReal						m_nominalOilTemperatureCelsius;

			smReal						m_oilTemperatureDropPerKnot;

            /** */
			virtual bool			    readParamStr( const std::string& parameterString , std::string& error );
	
		};

		/** 
			Creates an instance of GenericDriveTrain.
			\param vehicle		:	The vehicle to which the drive train is attached.
			\param name			:   Name of the drive train instance.
			\param params		:	Configuration parameters of the drive train component.
		*/
		static	DriveTrainModelBase*				createClass( RTD::VehicleBase* vehicle, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize a GenericDriveTrain instance.
		*/
		static	DriveTrainModelBase::ConfigParams*	createParameters();

		/** 
			Creates an instance of class.
			\param vehicle		:	The vehicle to which the drive train is attached.
			\param name			:   Name of the drive train instance.
			\param params		:	Configuration parameters of the drive train component.
		*/
		GenericDriveTrain( RTD::VehicleBase* vehicle, const std::string& name , const ConfigParams* params );

        /** Destructor of the class. */
		virtual ~GenericDriveTrain();


		/** Returns the type of the engine. For this model it returns "GenericDriveTrain". */
		virtual std::string		getType() const;


		/** Resets the engine. */
		virtual void			reset();

        /** */
		virtual void			update( smTime stepTime );

        /** Returns the configuration parameters of the model. */
		virtual const	ConfigParams*   getConfigParams() const;

        /** Returns the configuration parameters of the model. */
		virtual ConfigParams*   getConfigParams();

		/**
            This method is used to implement drive train malfunctions by severing the connection between a component and the 
            drive train. E.g. to simulate a main rotor drive failure following call is used:

            setDriveTrainConnectionStatus("MainRotor" , false );


        */
		virtual void			setDriveTrainConnectionStatus( const std::string& componentName , bool isIntact );
	
		/**
            Returns the actual state of a connection between the drive train and another component. If the given component is not
            connected to the drive train, false is returned.

            \see setDriveTrainConnectionStatus()
        */
		virtual bool		    getDriveTrainConnectionStatus( const std::string& componentName  ) const;

        /** */
		virtual smReal			getMaxContinuousTorqueLimit() const;

		/** */
		virtual smReal			getTakeoffTorqueLimit() const;

		/** */
		virtual smReal			getEmergencyTorqueLimit() const;

		/**
			This method returns the rate of revolution of the common engine shaft, i.e.
			the shaft to which all engines are connected.
		*/
		virtual smReal			getCommonEngineShaftRPM() const;

		/**
            Returns the actual gear box oil pressure in Bar.
        */
		smReal					getGearboxOilPressureBar() const;

		/**
            Returns the actual gear box oil temperature in degrees celcius.
        */
		smReal					getGearboxOilTemperatureCelsius() const;

	protected:

		ConfigParams	m_configParams;
				
		StateVariableDouble		s_CESAngularSpeed;
		StateVariableDouble		s_CESAngularSpeedPrev;
		StateVariableDouble		s_FSAngularSpeed;
		StateVariableDouble		s_FSAngularSpeedPrev;
		StateVariableDouble		s_GISAngularSpeed;
		StateVariableDouble		s_GISAngularSpeedPrev;
		OutputVariableReal		o_gearboxOilPressureBar;
		StateVariableReal		s_gearboxOilTemperature;

		std::vector< StateVariableDouble* > m_differential2PinionAngularSpeeds;

		struct ConnectedComponentInfo
		{
			ConnectedComponentInfo():m_name(""),m_component(NULL),m_isConnectionIntact(true)
			{

			};

			std::string					m_name;
			TraitDriveTrainConnection*	m_component;
			bool						m_isConnectionIntact;

		};

		std::vector<ConnectedComponentInfo>	m_connectedComponents;


		enum ClutchOperationMode
		{
			STICK = 0,
				SLIP,
				FREEWHEEL
		};

		StateVariableUInt		s_clutchOperationMode;

		virtual			void	solveStickedMode(	const smDouble& Kgb , 
													smDouble& dWces , smDouble& dWfs , smDouble& Tclutch , smDouble& Tdelivered );

		virtual			void	solveSlippingMode(	const smDouble& Kgb , 
													smDouble clutch,
													smDouble& dWces , smDouble& dWfs , smDouble& Tclutch , smDouble& Tdelivered );

		virtual			void	solveFreewheelingMode(	const smDouble& Kgb , 
														smDouble& dWces , smDouble& dWfs , smDouble& Tclutch , smDouble& Tdelivered );


		virtual			void	solveDifferential2( unsigned int index ,const smDouble& Tin, smDouble stepTime , smDouble& rightWheelAngularSpeed ,  smDouble& leftWheelAngularSpeed );

		virtual			void	computeLoadEquivalentTorqueAndInertia( smDouble& Tloads , smDouble& Iloads );
		virtual			void	computeDifferential2EquivalentTorqueAndInertia( unsigned int differentialIndex , smDouble& Teq , smDouble& Ieq );
		virtual			void	computeEngineEquivalentTorqueAndInertia( smDouble& Tengines , smDouble& Iengines );				

	};


}