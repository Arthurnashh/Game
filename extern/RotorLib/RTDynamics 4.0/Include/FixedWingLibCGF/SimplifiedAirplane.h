/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once


#include "RTD/MBDVehicle.h"
#include "RTD/LookupTable.h"



namespace RTD {
    class FGForce;
    class FGTorque;
}

namespace FixedWingLibCGF
{


    /**
    *   The SimplifiedAirplane is a generic CGF airplane flight dynamics model that is controllable by 
	*	an AirplaneBot. SimplifiedAirplane is a special flight dynamics model that enables higher performance 
	*	by reducing dynamics computations and control algorithm complexity.
    *
    *   SimplifiedAirplane can be configured using an XML based configuration file. Flight
    *   characteristics of different airplanes can be modeled with different XML files.
	*
	*	@see Flight Dynamics Model section of the FixedWingLib CGF chapter in the Programmer's Guide.
    */
    class RTDYNAMICS_EXPORT SimplifiedAirplane : public RTD::MBDVehicle
    {
    public:
        /**
            This class contains the configuration parameters of the SimplifiedAirplane.	The parameters that are defined here 
			can be configured using an XML file.
        */
        class RTDYNAMICS_EXPORT ConfigParams : public RTD::VehicleBase::ConfigParams
        {
        public:
            /** Constructor of the class. */
            ConfigParams();

			/** 
				@name Parameters

				Explanation of the following parameters is provided in the Programmer's Guide. See "Flight Dynamics Model (FDM)"
				section in the "FixedWingLib CGF" chapter.
			*/
			
			//@{
            smReal                  m_liftCurveSlope1;
            smReal                  m_maxLiftCoeff1;
            smReal                  m_liftCurveSlope2;
            smReal                  m_maxLiftCoeff2;
            smReal                  m_liftRefArea;

			smReal                  m_sideforceCurveSlope;
            smReal                  m_maxSideforceCoeff;
            smReal                  m_sideforceRefArea;

			smReal                  m_dragCoeff;
			smReal                  m_deltaCdsupersonic;
            smReal                  m_dragRefArea;
			smGraph2D				m_dragCoefficientCurve1;
			smGraph2D				m_dragCoefficientCurve2;
			smReal					m_transonicRegionBegin;
			smReal					m_transonicRegionEnd;

			smReal					m_flapExtensionTime;

			#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
			smReal					m_nozzleDirectionChangeSpeed;
			smReal					m_engineNozzleDirectionLimit;
			#endif

            smReal                  m_maxEngineThrust;

			RTD::LookUpTable		m_engineThrustTable;

            smGraph2D               m_maxRollRateVsSpeed;
            smGraph2D               m_maxPitchRateVsSpeed;

			smReal					m_maxLoadFactor;
			smReal					m_minLoadFactor;

			smReal					m_AoAlimiterActivationPoint;

			smReal                  m_aileronEffectiveness;
			smReal					m_aileronMinDeflection;
			smReal					m_aileronMaxDeflection;
			smReal					m_aileronRateLimit;

            smReal                  m_elevatorEffectiveness;
			smReal					m_elevatorMinDeflection;
			smReal					m_elevatorMaxDeflection;
			smReal					m_elevatorRateLimit;

            smReal                  m_rudderEffectiveness;
			smReal					m_rudderMinDeflection;
			smReal					m_rudderMaxDeflection;
			smReal					m_rudderRateLimit;

			smReal					m_airbrakeDragCoeff;
			smReal					m_airbrakeRefArea;
			smReal					m_airbrakeMaxDeflection;
			smReal					m_airbrakeExtensionSpeed;
			//@}


            /**
                A utility method that parses a character stream which contains initialization
                parameters. The \e parameterString is in XML. If there is an error \e false
                is returned and \e error will contain the reason.

                SimplifiedAirplane::ConfigParams::load() method uses this method to parse the parameter file.
            */
            virtual bool    readParamStr( const std::string& parameterString , std::string& error );
        };


        /**
            This constructor allows users to use a ConfigParams class directly.

            \param world    :   An instance of the RTD::World class which stores environmental information like wind or gravity.                                

			\param name			A unique name for the aircraft. If there is another vehicle with the same name
								within the same world, the newly created aircraft will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

            \param params   :   \e params contains the configuration parameters of the airplane. This pointer will
								\e not be deleted automatically.
        */
        SimplifiedAirplane( RTD::World* world , const std::string& name,  ConfigParams* params );

        /**
            This constructor is used to initialize the class with an XML file.

            \param world    :   An instance of the RTD::World class which stores environmental information like wind or gravity.

			\param name			A unique name for the aircraft. If there is another vehicle with the same name
								within the same world, the newly created aircraft will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

            \param parameterFile :  Name of an XML based configuration file. When this constructor is called, SimplifiedAirplane
                                    will be initialized with this configuration file. If there is a problem in reading the file,
									the airplane will be disabled. The format of this configuration file is explained in the
									FixedWingLib CGF section of the Programmer's Guide.
									
			@see isVehicleCorrectlyConfigured()

        */
        SimplifiedAirplane( RTD::World* world , const std::string& name, const std::string& parameterFile  );

        virtual void    preUpdate( smTime stepTime ) ;
        virtual void    postUpdate( smTime stepTime ) ;
        virtual void    reset();

        /** Returns current drag force in Newtons. Returned vector is in body coordinates. */
        const smVec3r&  getDrag() const {   return c_dragForce; }

        /** Returns current lift force in Newtons. Returned vector is in body coordinates. */
        const smVec3r&  getLift() const {   return c_liftForce; }

        /** Returns current side force in Newtons. Returned vector is in body coordinates. */
        const smVec3r&  getSideforce() const {   return c_sideForce; }

        /** Returns current engine thrust in Newtons. Returned vector is in body coordinates. 
			Note that, in case of a VTOL aircraft, thrust is not necessarily aligned with the 
			forward axis.
		*/
        const smVec3r&  getThrust() const   {   return c_engineThrust;  }

        /** Returns current angle of attack in degrees. */
        smReal          getAoA() const  {   return o_angleOfAttack;   }

        /** Returns the configuration parameters of the airplane. */
        const ConfigParams* getConfigParams() const     {   return m_configParams;  }

        /** Returns the configuration parameters of the airplane. */
        ConfigParams*       getConfigParams()           {   return m_configParams;  }

        /** Returns the force generator of this airplane. */
        RTD::FGForce*   getForceGenerator()             {   return m_forceGenerator ; }

        /** Returns the torque generator of this airplane. */
        RTD::FGTorque*  getTorqueGenerator()            {   return m_torqueGenerator; }

		/** Maximum pitch rate of an airplane depends on several different factors such as speed or air density. 
			This method returns the maximum	pitch rate that is currently possible in deg/s.*/
		smReal			getCurrentMaxPitchRate() const;

		/** Maximum roll rate of an airplane depends on several different factors such as speed or air density. 
			This method returns the maximum	roll rate that is currently possible in deg/s.*/
		smReal			getCurrentMaxRollRate() const;


		/** Maximum thrust that can be produced by an engine depends on several different factors such as speed 
			or air density. This method returns the maximum	thrust that is currently possible in Newtons.*/
		smReal			getCurrentMaxThrust() const;

		/** This method returns the throttle setting that will generate a given thrust. \e thrust is in Newtons.
			A return value of 0 means 0% throttle and 1 means 100% throttle. */
		smReal			getThrottleForGivenThrust( smReal thrust ) const;

		/** This method returns the maximum pitch rate that is possible under the most optimal conditions in deg/s. */
		smReal			getMaxPitchRate() const;

		/** This method returns the maximum roll rate that is possible under the most optimal conditions in deg/s. */
		smReal			getMaxRollRate() const;

		/** This method returns the maximum load factor allowed for the airplane in terms of g's. Note that this is \e 
			not necessarily	the maximum load factor that is aerodynamically possible. Maximum load factor is limited by
			other factors such as structural integrity limits.*/
		smReal			getMaxLoadFactor() const;

		/** This method returns the minimum load factor allowed for the airplane in terms of g's. Note that this is \e 
			not necessarily	the minimum load factor that is aerodynamically possible. Minimum load factor is limited by
			other factors such as structural integrity limits. The returned value is normally negative.*/
		smReal			getMinLoadFactor() const;


		/** This method returns the AoA at which the AoA limiter is activated to limit the maximum AoA. 			
			The returned value is in degrees. */
		smReal			getAoALimiterActivationPoint() const;

        /** This method returns the maximum speed allowed for the airplane in Knots. Note that this is \e 
			not necessarily	the maximum speed that is aerodynamically possible. Maximum speed is limited by
			other factors such as structural integrity limits.*/
		smReal			getMaxSpeedKts() const;
		

		/** Returns the deflection angle of the elevators in degrees. Deflection is positive when 
			the trailing edge of the elevators is down. 
		
			This method is used for control surface animation.
		*/		
		smReal			getElevatorDeflection() const;

		/** Returns the deflection angle of the ailerons in degrees. Deflection is positive when
			the trailing edge of the right-wing ailerons are down.
					
			This method is used for control surface animation.
		*/
		smReal			getAileronDeflection() const;

		/** Returns the deflection angle of the rudder in degrees. Deflection is positive when
			the trailing edge of the rudder is deflected to the left.
					
			This method is used for control surface animation.
		*/
		smReal			getRudderDeflection() const;

		/** Returns the deflection angle of the air brakes in degrees. When	airbrakes are fully 
			retracted, deflection is zero. 
					
			This method is used for control surface animation.
		*/
		smReal			getAirbrakeDeflection() const;

		/**
            This method returns the actual slope of the lift coefficient which depends on
            current flap settings.
        */
		smReal			getCurrentLiftCoefficientSlope() const;
		
        /**
            This method returns the actual maximum lift coefficient which depends on
            current flap settings.
        */
		smReal			getCurrentMaxLiftCoefficient() const;

        /**
            This method returns the actual lift coefficient which depends on
            current flap settings and angle of attack.
        */
		smReal			getCurrentLiftCoefficient() const;

        /**
            This method returns the actual drag coefficient which depends on
            current flap settings, angle of attack and mach number.
        */
        smReal			getCurrentCoefficientOfDrag() const;

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
		smReal			getNozzleDirection() const;

		void			setNozzleDirection( smReal d ) ;
#endif

		void			enableExtendedFlightModel( bool enable );
    protected:
        /**
            The create method is called by the constructor of the SimplifiedAirplane.
        */
        virtual void        create();

    protected:
        ConfigParams*               m_configParams;

        RTD::FGForce*       m_forceGenerator;
        RTD::FGTorque*      m_torqueGenerator;

        smReal  c_AoA;
        smReal  c_sideslip;
        smVec3r c_liftForce;
        smVec3r c_dragForce;
		smVec3r c_sideForce;
        smVec3r c_engineThrust;

		RTD::StateVariableVec3r	s_Mcompensation_bcs;

		// Positive for trailing edge down
		RTD::StateVariableReal	s_normalizedAileronDeflection;

		// Positive for right-wing trailing edge down
		RTD::StateVariableReal	s_normalizedElevatorDeflection;

		// Positive for trailing edge left
		RTD::StateVariableReal	s_normalizedRudderDeflection;

		// Positive when extended. 0 when completely retracted.
		RTD::StateVariableReal	s_normalizedAirbrakeDeflection;

        RTD::StateVariableReal	s_prevSideslip;

		RTD::StateVariableReal	s_flapState;

		RTD::StateVariableReal	s_nozzleDirection;

		bool	m_extendedFlightModelEnabled;

		/**
        *   Destructor of the class. This method is intentionally protected; use
        *   remove() method instead.
        */
		virtual ~SimplifiedAirplane( void );

        virtual void    updateDataLogger();

		virtual void	updateNormalizedControlSurfaceDeflections( smTime deltaTime ); 
		virtual void	updateFlaps( smTime timestep );

		#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
		virtual void	updateNozzleDirection( smTime timestep );
		#endif

    };

}
