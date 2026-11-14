/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/RigidBody.h"
#include "RTD/RigidBodyState.h"
#include "RTD/TraitEngine.h"
#include "RTD/TraitControlSystem.h"
#include "RTD/TraitGroundContact.h"
#include "RTD/Instruments.h"
#include "RTD/ForceGenerator.h"
#include "RTD/WorldParticipant.h"
#include "RTD/InputData.h"
#include "RTD/DataLogger.h"
#include "RTD/Math.h"
#include "RTD/EarthModelWGS84.h"
#include "RTD/TraitDriveTrain.h"
#include "RTD/Visualization.h"

namespace RTD
{
	class ReferenceVehicle;
	class FuelConsumptionModelBase;
	class WindsSystem;

    /**
    *   Interface base class for all vehicles. This class defines common methods for all vehicle
    *   types as well as common properties of vehicles.
    */
    class RTDYNAMICS_EXPORT VehicleBase :   public WorldParticipant,
											public SimulationVariableUser,
                                            public TraitEngine,
                                            public TraitControlSystem,
                                            public TraitGroundContact,
											public TraitDriveTrain,
                                            public TraitStdInstruments
    {
		friend class ModelComponentBase;
    public:
        /** Common configuration parameters for all vehicles. */
        class RTDYNAMICS_EXPORT ConfigParams
        {
			
        public:			

            /** Constructor of the class.   */
            ConfigParams();


            /** Initial mass of the Main Body in kilograms. 

                Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.

                \see VehicleBase::setMainBodyMass()

            */
            smDouble              m_mass ;

            /** 
                Initial moments of inertia of the Main Body in kg*m^2. With Ixx, Iyy, Izz and Iyz order.

                Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.

                \see VehicleBase::setMainBodyMomentsOfInertia()
            */
            smVec4d               m_momentsOfInertia;

			/**
                Initial position of the center of gravity of the Main Body in body coordinates. This vector is in meters.
                
                Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.

                \see VehicleBase::setMainBodyCenterOfGravityBCS 
            */
			smVec3d               m_centerOfGravityBCS;

            /**
                This vector contains a list of engine model class names. When a vehicle
                is initialized it will create an engine for each entry in the vector.
                The string obtained from m_engineClassNames defines which class should
                be instantiated. After creating a new instance of the given class it will be
                initialized using the parameters given in m_engineParams.

                - The engine class should be registered to be recognized (see RTD::EngineModelBase::registerClass() )
                - The number of entries should be equal to the number of entries in m_engineParams
                    variable.
                - The parameter class which is stored in m_engineParams should match the
                    classes which are defined in m_engineClassNames. For example if m_engineClassNames[1]
                    is "SimpleThrustEngine" then m_engineParams[1] should be a pointer to
                    SimpleThrustEngine::ConfigParams.

                @see RTD::EngineModelBase
                @see m_engineParams
            */
            std::vector<std::string>    m_engineClassNames;

            /**
                This vector stores configuration parameters of engines that are going to be
                created during initialization. See m_engineClassNames for details.
            */
            std::vector<RTD::EngineModelBase::ConfigParams*>    m_engineParams;

            /** This vector contains a list of control system component class names. When a vehicle
                is initialized it will create an control system component for each entry in the vector.
                The string obtained from m_controlSystemComponentClassNames defines which class should
                be instantiated. After creating a new instance of the given class it will be
                initialized using the parameters given in m_fcsComponentParameters.

                - The control system component class should be registered to be recognized (see ControlSystemComponentBase::registerClass() )
                - The number of entries should be equal to the number of entries in m_fcsComponentParameters
                  variable.
                - The parameter class which is stored in m_fcsComponentParameters should match the
                  classes which are defined in m_fcsComponentClassNames. For example if m_fcsComponentClassNames[1]
                  is "FCSRotationDamper" then m_fcsComponentParameters[1] should be a pointer to
                  FCSRotationDamper::ConfigParams.

                @see ControlSystemComponentBase
                @see m_fcsComponentParameters
            */
            std::vector<std::string>    m_controlSystemComponentClassNames;

            /**
                This vector stores configuration parameters of control system components that are going to be
                created during initialization. See m_controlSystemComponentClassNames for details.
            */
            std::vector<RTD::ControlSystemComponentBase::ConfigParams*> m_controlSystemComponentParameters;


            /** Drag coefficient of the landing gears. Gears create air drag only when they are extended. */
            smReal          m_landingGearDragCoeff;

            /** The reference cross section area of the gears to be used in air drag computation. In m^2 */
            smReal          m_landingGearReferenceArea;

            /**
            How long it takes to extend the landing gears fully. The time needed to
            retract the gears assumed to be the same. The value is in seconds.
            */
            smReal          m_landingGearExtensionTime;

            /**
            This vector contains a list of ground contact model class names. When an vehicle
            is initialized it will create a ground contact model for each entry in the vector.
            The string obtained from m_groundContactParams defines which class should
            be instantiated. After creating a new instance of the given class it will be
            initialized using the parameters given in m_groundContactParams.
            */
            std::vector<std::string>    m_groundContactClassNames;

            /**
            This vector stores configuration parameters of ground contact models that are going to be
            created during initialization. See m_groundContactClassNames for details.
            */
            std::vector<RTD::FGGroundContactBase::ConfigParams*>    m_groundContactParams;


			std::vector<std::string>	m_driveTrainClassNames;

			std::vector<RTD::DriveTrainModelBase::ConfigParams*>	m_driveTrainParameters;


            /**
            @name Steering parameters

            Steerable gears are controlled with pedal input. The amount that the gear
            can be turned decreases with the square of the speed so that the gear is not
            turned too much when the vehicle is too fast. The formula is:

            (pedals * m_maximumSteerableGearDeflection ) / (1 + speed^2 *m_steerableGearSpeedCorrectionRatio );

            m_maximumSteerableGearDeflection is a positive value in degrees.
            */
            //@{
            smReal              m_steerableGearSpeedCorrectionFactor;
            smReal              m_maximumSteerableGearDeflection;
            //@}


            /** 
				Position and orientation of object attachment stations.
            */
            struct AttachmentStationInfo
            {
                smVec3r m_xyz;
                smVec3r m_hpr;
            };

			/**
				RTD simulation library allows attaching objects to other objects and vehicles, for example,
				bombs to aircrafts or loads to a truck.

				Attachments can be done at any location; however, usually certain locations on the vehicle body
				has the purpose of attaching objects. E.g. pylons on a bomber wing. This parameter is used to
				store the information about such special object attachment points. Note that having this information
				as a configuration parameter also allows reading this information from a configuration file.
			*/
            std::vector<AttachmentStationInfo>    m_attachmentStations ;


            /** Name of the simulation log file. If the file name is set to "" (default) no loggin will
                take place. */
            std::string         m_simulationLogFile;

            smReal          m_stableHeadingAccelerationThreshold;
            smReal          m_stableHeadingRateHistoryMaxThreshold;
            smReal          m_stablePitchRateHistoryMaxThreshold;
            smReal          m_stableRollRateHistoryMaxThreshold;
            smReal          m_stableSpeedVelocityThreshold;
            smReal          m_stableSpeedAccelerationThreshold;
            smReal          m_stableAltitudeVelocityDirThreshold;
            smReal          m_stableAltitudeRateHistoryMaxThreshold;



            /**
                This method loads a parameter file which is used to initialize the vehicle
                dynamics model.

                If there is an error in loading the file or parsing the contents, \e false is
                returned and \e error will contain the reason.
            */

			virtual bool	load( const std::string& parameterFile , std::string& error );

            /**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
            */
            virtual bool    readParamStr( const std::string& parameterString , std::string& error );

			/**
				This method is used to delete the ConfigParams instance. Using delete operator is 
				not possible because the the destructor is defined protected. 
			*/
			void	remove();

        protected:
            /** Copy constructor of the class. Deep copying is not supported and can result in
            hard to catch bugs if used. Because of this copy constructor is disabled. */
            ConfigParams( const ConfigParams& rhs );

            /** Assignment operator of the class. Deep copying is not supported and can result in
            hard to catch bugs if used. Because of this copy assignment operator is disabled. */
            virtual ConfigParams&   operator=(const ConfigParams& rhs );

            /** Do not allow a ConfigParams instance on the stack. ConfigParams should be a pointer
            always. */
            virtual ~ConfigParams();

        };

        /**
            Damage state of the vehicle
        */
        enum DamageState
        {
            Undamaged = 0,  /*!< the vehicle is undamaged */
            Damaged,    /*!< the vehicle is damaged, the pilot inputs will not be working properly anymore */
            Crashed     /*!< the vehicle has crashed, the physics simulation update stops completely. */
        };

        /**
            Constructor of the class.

            \param  world   :   An instance of World class which stores environmental information like wind,
                                air pressure vs. altitude curve etc.
            \param name     :   A unique name for the vehicle. If there is another vehicle with the same name
								within the same world, the newly created vehicle will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.
        */
        explicit VehicleBase( World* world, const std::string& name ) ;

        /** Call this method to run the simulation for the given time step.
        *   \param  timestep    time step in seconds
        */
        virtual void update( smTime timestep );

        /** Enables/disables this vehicle. When disabled, the vehicle remains
         *  unchanged during simulation.
         */
        virtual void setEnabled( bool enabled );

		/** Returns true if this vehicle is enabled.   */
        virtual bool isEnabled( );

        /** Get the force generators on this vehicle. */
        const smManager<ForceGenerator>&    getForceGenerators() const;

        /** Get the force generators on this vehicle. */
        smManager<ForceGenerator>&  getForceGenerators();


        /** \page WeightBalanceInertia Weight, balance and inertia

            RTDynamics SDK has a rigid body simulation engine on which all other simulation components are build. This rigid body
            engine allows for defining multiple rigid bodies, which are connected to each other with various joint types, to represent
            a vehicle.

            Initially a single rigid body that represents the vehicle is created. This rigid body is referred to as the "Main Body" and it 
            is initialized with the mass, inertia and center of gravity related configuration parameters provided in the ConfigParams
            class or the XML configuration file. Whenever a reset occurs, the Main Body is reset to this initial configuration.

        */
        

		/** Set the mass of the Main Body in kilograms.
            
            Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.
        */
		virtual void				setMainBodyMass( smDouble mass );

		/** Set the moment of inertia of the Main Body. Members of the input vector are Ixx,Iyy,Izz and Iyz. 
                    Moments of inertia is defined in kg*m^2. 
                    
            Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.
       */
		virtual void				setMainBodyMomentsOfInertia( const smVec4d& moi );

		/**
            Set the Main Body center of gravity in body coordinates. The position is defined in meters. 

            Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.
        */
		virtual void				setMainBodyCenterOfGravityBCS( const smVec3d& cogBCS );

		/**
            Returns the Main Body mass in kg.

            Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.
        */
		virtual smDouble			getMainBodyMass() const;

		/**
            Returns the Main Body moments of inertia in kg in and Ixx,Iyy,Izz and Iyz order.

            Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.
        */
		virtual smVec4d				getMainBodyMomentsOfInertia() const;

		/**
            Returns the position of the center of gravity of the Main Body in meters and in body coordinates.

            Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.
        */
		virtual smVec3d				getMainBodyCenterOfGravityBCS() const;
        
		/** Returns the center of gravity in body coordinates. Returned vector takes the Main Body, landing gears and external attachments into account.
        
            Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.
        */
		virtual const smVec3d&		getCenterOfGravityBCS() const;

		/** Returns the total mass including the Main Body, landing gears and external attachments (e.g. weapons). Return value is in kilograms 
        
            Please refer to \ref WeightBalanceInertia for the explanation of the Main Body concept.
        */
		virtual smDouble			getTotalMass() const;

        


        /** Reset this vehicle. If derived classes override this method, they shall
        *   call RTD::VehicleBase::reset() in their reset implementation.
        *  
        *   Disabled vehicles will be automatically enabled after reset.
        */
        virtual void    reset();

        /** Get state as read only.                     */
        const RigidBodyStateBase&   getState() const;


        /** @name Dynamics state access

            Following methods are used to access the current state of
            the vehicle.            
        */
        //@{
        virtual void                    setPosition( const smVec3r& p );
        virtual smVec3r		            getPosition() const;
        virtual void                    setRotation( const smAngleDVec& hpr ) ;
        virtual const smMatrix&         getRotation() const;
        virtual void                    setVelocity( const smVec3r& v );
        virtual smVec3r			        getVelocity() const;
        virtual void                    setRotVelocityBCS( const smVec3r& w );

        virtual smVec3d				    getPositionD() const;


        /** getAltitudeInFeet() is a convenience method which returns
         *  the current altitude (z component of the position) in
         *  feet. */
        virtual smReal                  getAltitudeInFeet() const ;

		/** getIndicatedAirSpeedKnots() is a convenience method which returns
         *  the current indicated air speed in knots. */
		virtual smReal                  getIndicatedAirSpeedKnots() const ;

        /** Returns angular velocity (angular speeds around x,y and z axes as rad/s) vector in body coordinate system. */
        virtual smVec3r					getRotVelocityBCS() const;

        /** Returns angular velocity (angular speeds around x,y and z axes as rad/s) vector in world coordinate system. */
        virtual smVec3r					getRotVelocityWCS() const;

        virtual void                    setAcceleration( const smVec3r& accel );
        virtual smVec3r			        getAcceleration() const;

        virtual void                    setRotAccelerationBCS( const smVec3r& rotaccel );

        /** Returns angular acceleration (angular acceleration around x,y and z axes in rad/s^2) vector in body 
			coordinate system. */
        virtual smVec3r					getRotAccelerationBCS() const;
  
		/** Returns angular acceleration (angular acceleration around x,y and z axes in rad/s^2) vector in world 
			coordinate system. */  
        virtual smVec3r			        getRotAccelerationWCS() const;


        /** Returns rotational acceleration as yaw rate, pitch rate and roll rate (in the same order as
           given here) in deg/s. Returned angular rates are in body coordinate system. */
        virtual smVec3r                 getRotVelocityHPR() const;

        /** Returns rotational acceleration as yaw acceleration, pitch acceleration and roll acceleration
            (in the same order as given here) in deg/s^2. Returned accelerations are in body coordinate system. */
        virtual smVec3r                 getRotAccelerationHPR() const;


        /** Internally RTD uses row-major matrices as defined in algebra,
        *   however OpenGL applications (e.g. OSG) use column-major matrices. Use
        *   getRotationGL() to get rotation matrix in column-major format.
        *
        *   This matrix is in world coordinate system.
        */
        virtual const smMatrix&         getRotationGL() const;

        /** Returns the current rotation in degrees in HPR (heading-pitch-roll) convention. Returned attitude is
            in world coordinate system.*/
        virtual smVec3r					getRotationAsHPR() const;

        //@}


        /** @name Direction Vectors
		*		  These methods return vectors pointing to the
        *         front, right and up of the vehicle respectively. Returned vectors
		*		  are in world coordinates.
        */
        //@{
        virtual void    getForwardVector( smVec3r& forward ) const;
        virtual void    getSidewardVector( smVec3r& sideward ) const;
        virtual void    getUpwardVector( smVec3r& upward ) const;
        //@}


		/** Returns the linear wind velocity vector in m/s. */
		virtual smVec3r			getLinearWindVelocity() const;

		/** Returns the rotational wind velocity vector in m/s. */
		virtual smVec3r			getRotationalWindVelocity() const;

        /** Returns the density of air at the current altitude. Returned value is in
            kg/m^3 .
        */
        smReal                  getAirDensity() const;

        /** getAoASideslip computes the angle of attack and the side slip angles as defined
        *   in aerospace engineering literature. It takes the wind into account.
        *
        *   @param  AoA         AoA is usually around 0 degrees. Positive AoA means headwind facing
        *                       the bottom of the aircraft.
        *   @param  sideslip    Sideslip is also usually around 0 degrees. Positive sideslip means headwind
        *                       facing the right side (starboard) of the the aircraft.
        */
        void                    getAoASideslip( smReal& AoA, smReal& sideslip  ) const  ;

        /** Returns the damage state of the vehicle. */
        const DamageState       getDamageState()                        { return (DamageState)s_damageState.getValue(); }

        /** Sets the damage state of the vehicle.  */
        virtual void            setDamageState(const DamageState& dstate)       { s_damageState.setValue( dstate ); }

        /** Returns InputData structure. */
        InputData*              getInputData()          { return m_inputData ; }

        /** Returns InputData structure. */
        const   InputData*      getInputData() const    { return m_inputData ; }

        /** Replaces current InputData with a user provided one and deletes the current one. */
        virtual void            setInputData(InputData*);


        /** getPilotInputData() is a utility method that returns InputData after casting it to
            PilotInputData. If InputData is not a PilotInputData NULL is returned.*/
        PilotInputData*         getPilotInputData();

        /** getPilotInputData() is a utility method that returns InputData after casting it to
            PilotInputData. If InputData is not a PilotInputData NULL is returned.*/
        const   PilotInputData* getPilotInputData() const;

        /** getDriverInputData() is a utility method that returns InputData after casting it to
            DriverInputData. If InputData is not a DriverInputData NULL is returned.*/
        DriverInputData*            getDriverInputData();

        /** getDriverInputData() is a utility method that returns InputData after casting it to
            DriverInputData. If InputData is not a DriverInputData NULL is returned.*/
        const   DriverInputData*    getDriverInputData() const;

        /** Returns InputData structure. */
		WindsSystem*              getWindsSystem()          { return m_windsSystem ; }

        /** Returns false if the configuration file or data was invalid or erroronous.*/
        virtual bool            isVehicleCorrectlyConfigured() { return m_isVehicleCorrectlyConfigured ; }

        /** Positions the vehicle at the given geographic coordinates. The position of the
        *   vehicle in world coordinate system (WCS) is computed from the given geographic
        *   position and the surface center of the world.
        *
        *   @param lla           [in] latitude [rad], longitude [rad], altitude
        *                        [m]
        *   @param offset        [in] optionally you can specify an offset to
        *                        the geographic position. If you wish to place
        *                        the vehicle at an airport, you can provide the
        *                        geographic position of the aircraft and use the
        *                        offset to place the vehicle more precisely
        *                        (e.g. on the runway.) The offset is in WCS
        *                        coordinate system.
        *
        *   Also see World::setSurfaceCenter
        */
        void    setPositionWithLLA(const smVec3d& lla, const smVec3r& offset = smVec3r() );

        /** Use getPositionAsXYZECEF method to get the XYZ position of the vehicle in ECEF (earth centric earth fixed)
        *   coordinate system.
        *
        *   @param  xyz         position of the vehicle in ECEF coordinate system.
        *                       The worlds radius is about 6378137.0 meters, so this vector is supposed to
        *                       contain very large values.
        *
        *   See also getPositionAsLLA and World::setCenter
        */
        void    getPositionAsXYZECEF( EarthModelWGS84::VecD& xyz ) ;

        /** Use getPositionAsXYZECEF method to convert a position vector defined in WCS to ECEF (earth centric earth fixed)
        *   coordinate system.
        *
        *   @param  R_wcs       position vector defined in WCS.
        *   @param  xyz         position of the vehicle in ECEF coordinate system.
        *                       The worlds radius is about 6378137.0 meters, so this vector is supposed to
        *                       contain very large values.
        *
        *   See also getPositionAsLLA and World::setCenter
        */
        void    getPositionAsXYZECEF( smVec3d R_wcs, EarthModelWGS84::VecD& xyz ) ;

        /** Use getPositionAsLLA method to get the geographic coordinates (LLA) of the vehicle.
        *
        *   @param  lat,lon,alt the computed geographic coordinates
        *
        *   See also getPositionAsXYZECEF and World::setCenter
        */
        void    getPositionAsLLA( EarthModelWGS84::Double& lat,
                                  EarthModelWGS84::Double& lon,
                                  EarthModelWGS84::Double& alt ) ;

        /** Use getPositionAsLLA method to get the geographic coordinates (LLA) of the vehicle.
        *   @return a 3d vector with components [lat(rad),lon(rad),alt(m)]
        */
        smVec3d getPositionAsLLA( ) ;

        /** Use getPositionAsLLA method to convert a position vector defined in WCS to geographic coordinates (LLA).
        *
        *   @param  R_wcs       position vector defined in WCS.
        *   @param  lat,lon,alt the computed geographic coordinates
        *
        *   See also getPositionAsXYZECEF and World::setCenter
        */
        void    getPositionAsLLA( const smVec3d& R_wcs,
                                  EarthModelWGS84::Double& lat,
                                  EarthModelWGS84::Double& lon,
                                  EarthModelWGS84::Double& alt ) ;

        /** Use getPositionAsUTM method to get the position of vehicle in UTM
        *   coordinate system.
        *
        *   @param  UTMNorthing UTM northing coordinate in meters [in]
        *   @param  UTMEasting  UTM easting coordinate  in meters [in]
        *   @param  UTMZone     String value of the UTM zone [in]
        *
        *   Also see UTM
        */
        void getPositionAsUTM(EarthModelWGS84::Double &UTMNorthing,
                              EarthModelWGS84::Double &UTMEasting,
                              std::string& UTMZone);


        /** Sets the simulation log file's name. If file name is "", no log file be created. 
        *   When this method is called with a valid file name, a new file will be
        *   created and logging will start. To stop \e setSimulationLogFile should
        *   again be called with "".
        *
        *   \e downsamplingFactor is used to control the size of the output file. if it is more than
        *   1, some of the simulation steps will be omitted. E.g. if it is 10 every 10th simulation
        *   step will be saved to the file.
        *
        *   \e activeChannels is used to limit the channels for which data is collected. If the vector is empty,
        *   which is the default, then all channels are logged. If some channels names are provided than only
        *   these channels will be logged. This helps to reduce the memory footprint of the logger and also
        *   keeps log file small.
        *
        *   \e binaryFileFormat Reserved for future use. Set to
        *                       DataLogger::FileFormat_BinaryVersion1.
        */
        virtual void    setSimulationLogFile( const std::string& filename
                                              , DataLogger::FileFormat logFormatType = DataLogger::FileFormat_BinaryVersion1
                                              , unsigned int downsamplingFactor = 1
                                              , const std::vector< std::string >& activeChannels = std::vector< std::string >() );

        /** Returns the name of the simulation log file. If returned value is "", then
            logging is disabled. */
        const std::string&  getSimulationLogFile() const    {   return m_simulationLogFileName; }

        /** Returns the data logger of the vehicle. The simulation log file which is set by setSimulationLogFile() is filled
			by this DataLogger object. */
        const DataLogger*   getDataLogger() const           {   return m_dataLogger;    }

        /** Returns the data logger of the vehicle. The simulation log file which is set by setSimulationLogFile() is filled
			by this DataLogger object. */
        DataLogger*         getDataLogger()                 {   return m_dataLogger;    }

        /** Returns a pointer to the Visualization object of this vehicle.  */
        const Visualization*   getVisualization() const           {   return m_visualization;    }

        /** Returns a pointer to the Visualization object of this vehicle.  */
        Visualization*         getVisualization()                 {   return m_visualization;    }

		/** Returns a reference object that references to this vehicle. */
		virtual	ReferenceVehicle*		getReference()			{	return m_reference;	}		
		
		/** Returns a reference object that references to this vehicle. */
		virtual	const ReferenceVehicle*	getReference() const	{	return m_reference;	}

		/** Returns the fuel consumption model, 0 is returned if there is no fuel consumption model specified. */
		virtual	FuelConsumptionModelBase*		getFuelConsumptionModel() {	return m_fuelConsumptionModel;	}
		
		/** Sets the fuel consumption model. The ownership of the model is passed to VehicleBase. 
		*	VehicleBase will delete the fuel consumption model in its destructor.
		*
		*	If there is a fuel consumption model specified, it will be deleted. 
		*/
		virtual	void							setFuelConsumptionModel(FuelConsumptionModelBase* model) ;

		/** Returns true if the heading of the aircraft is stable. This condition
		    is fulfilled if the heading change has been zero for a certain time. */
		virtual bool					hasStableHeading() const;
		virtual bool					hasStableHeading(const float& t1,const float& t2) const;

		/** Returns true if the pitch attitude of the aircraft is stable. This condition
		    is fulfilled if the pitch change has been zero for a certain time. 
		*/
		virtual bool					hasStablePitchAttitude() const;

		/** Returns true if the roll attitude of the aircraft is stable. This condition
		    is fulfilled if the roll change has been zero for a certain time. 
		*/
		virtual bool					hasStableRollAttitude() const;

		/** Returns true if the speed of the aircraft is stable. This condition
		    is fulfilled if the linear acceleration change has been zero for a certain time. 
		*/
		virtual bool					hasStableSpeed() const;

		/** Returns true if the altitude of the aircraft is stable. This condition
		    is fulfilled if the altitude change has been zero for a certain time. 
		*/
		virtual bool					hasStableAltitude() const;

		/**
			Returns true if the aircraft is in straight and level flight.
		*/
		virtual bool					isInLevelFlight() const;



		/** Returns configuration parameters of the VehicleBase instance.. */
		virtual ConfigParams*				getConfigParams() = 0;

		/** Returns configuration parameters of the VehicleBase instance.. */
		virtual const ConfigParams*			getConfigParams() const = 0;

		/**
			This method is used to take a snapshot of the current physical state of this entity. \e stateInfo
			is a byte array that contains the state information.

			@see deserialize()
		*/
		virtual void						serialize( std::vector<unsigned char>& stateInfo );

		/**
			This method is used to restore the physical state of the vehicle from a byte array that contains the physical state information. 
			
			\param stateInfo The byte array that contains the state information.
			\param remainder If the information contained in the \e stateInfo is more than the vehicle needs, unprocessed parts of the
					\e stateInfo will be copied to the \e remainder.

			@see serialize()
		*/
		virtual bool						deserialize( const std::vector<unsigned char>& stateInfo, std::vector<unsigned char>& remainder );


		/**
			This is a method that is used internally only.
		*/
		void								enableTrimMode( bool en ) 	{	m_isInTrimMode = en;	}


		/**
			This is a method that is used internally only.
		*/
		bool								isInTrimMode() const	{	return m_isInTrimMode;	}

		/**
            Returns the number of model components that are used in this vehicle.
        */
		unsigned int						getNumModelComponents() const;

		/**
            Returns the model component withe the given index. The method returns NULL if the index is out of range.
        */
		const ModelComponentBase*			getModelComponent( unsigned int index )	const;

		/**
            Returns the model component withe the given index. The method returns NULL if the index is out of range.
        */
		ModelComponentBase*					getModelComponent( unsigned int index );

		const RTD::RealDataHistory&			getSpeedHistory() const { return c_speedHistory; } 
		const RTD::RealDataHistory&			getLinearAccelerationHistory() const { return c_linearAccelerationHistory; } 


protected:
 
        smManager<ForceGenerator>   m_forceGens ;
		ReferenceVehicle*			m_reference;
		
        StateVariableUInt			s_damageState ;
		StateVariableVec3d			s_position ;		
		StateVariableVec3d			s_velocity ;		
		StateVariableVec4d			s_attitudeQuat ;		
		StateVariableVec3d			s_angularVelocityWCS ;		

		StateVariableDouble			s_mainBodyMass;
		StateVariableVec4d			s_mainBodyMomentsOfInertia;
		StateVariableVec3d			s_mainBodyCoGBCS ;
		StateVariableVec3d			s_mainBodyCoGWCS ;
		StateVariableVec4d			s_mainBodyCoGQuat ;
		StateVariableVec3d			s_mainBodyCoGVelocity;
		StateVariableVec3d			s_mainBodyCoGAngularVelocity;

        OutputVariableVec3d			o_velocityBCS ;
        OutputVariableVec3d			o_accelerationBCS;
        OutputVariableVec3d	        o_angularVelocityBCS;
        OutputVariableDouble        o_indicatedAirSpeedKts;
		OutputVariableVec3d			o_attitudeHPR ;
		OutputVariableVec3r			o_acceleration;
		OutputVariableVec3r			o_angularAccelerationBCS ;
		OutputVariableVec3r			o_angularAccelerationWCS ;
		OutputVariableReal			o_angleOfAttack;
		OutputVariableReal			o_sideslip;
		
		OutputVariableVec3r			o_vWindWCS; // Linear wind velocities in WCS frame. In m/s
		OutputVariableVec3r			o_vWindPQR;	// Angular wind velocities. In rad/s
		
		// True if the ConfigParams object is created by this object.
		bool						m_ownConfigParams;

        InputData*                  m_inputData;
        bool                        m_isVehicleCorrectlyConfigured;

        // All derived classes shall access state with getState and getStateToChange.
        mutable RigidBodyStateBase* m_state ;

        std::string             m_simulationLogFileName;
        DataLogger*             m_dataLogger;
		WindsSystem*            m_windsSystem;

		FuelConsumptionModelBase*	m_fuelConsumptionModel;

        Visualization*          m_visualization ;

		RTD::RealDataHistory	c_headingHistory;
		RTD::RealDataHistory	c_pitchHistory;
		RTD::RealDataHistory	c_rollHistory;

		RTD::RealDataHistory	c_headingRateHistory;
		RTD::RealDataHistory	c_pitchRateHistory;
		RTD::RealDataHistory	c_rollRateHistory;
		RTD::RealDataHistory	c_linearAccelerationHistory;
		RTD::RealDataHistory	c_verticalAccelerationHistory;
		RTD::RealDataHistory	c_speedHistory;

		mutable bool			m_massPropertiesDirty;
		mutable smDouble		m_totalMass;
		mutable smVec3d			m_centerOfMass;

		bool					m_isInTrimMode;

		std::vector<RTD::ModelComponentBase*>	m_modelComponents;

        /**
        *    Destructor of the class. The destructor is intentionally declared as
        *    a protected destructor. Use remove() method to delete an instance of
        *    this class. See WorldParticipant base class.
        */
        virtual ~VehicleBase();

		virtual void create();

        /** Get a reference to state with write access. */
        virtual RigidBodyStateBase& getStateToChange();


		virtual void			computeMassProperties() const = 0;

        virtual void            onExternalPhysicalStateChange();

        virtual void            updateDataLogger();
        virtual void            updateVisualization();

		/**
			This method is made protected to prevent that the name that is given in the constructor is
			changed later. Vehicle names are also unique ID's and changing them might create problems.
		*/
		virtual void setName( const std::string& name )  {}

		virtual void setMainBodyMassParamDefaultsAndValues();
    };
}