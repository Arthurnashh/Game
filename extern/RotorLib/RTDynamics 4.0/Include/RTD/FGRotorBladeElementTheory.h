/*
	(c) Copyright 2011, RTDynamics
	All Rights Reserved.

	Product:		RTDynamics
	

	The content of this work is proprietary and confidential.Permission to use,copy,
	publish and modify this software and its documentation for any commercial and
	non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/VehicleBase.h"
#include "RTD/ForceGenerator.h"
#include <Physics/smGraph2D.h>
#include "RTD/RotorModelBase.h"

namespace RTD
{
	class VehicleBase;
	class FGHeliFuselage100;

	/**
		FGRotorBladeElementTheory implements a high fidelity rotor model using momentum theory.

		@see FGRotorBladeElementTheory::ConfigParams
	*/
	class RTDYNAMICS_EXPORT FGRotorBladeElementTheory : public RotorModelBase
	{
	public:
		/**
			Configuration parameters of the rotor.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RotorModelBase::ConfigParams
		{
		public:

			ConfigParams();
			virtual ~ConfigParams();

			/**
				@name Rotor Geometry
			*/
			//@{
			/** Position at which rotor is attached to the vehicle. */
			smVec3r			m_position;

			/**
				Longitudinal tilt of the rotor in degrees. Positive values denote a backward tilt.
				This parameter is for most of the helicopters negative.
			*/
			smReal			m_longitudinalTilt;

			/**
				Lateral tilt of the rotor in degrees. Positive values denote a rightward tilt.				
			*/
			smReal			m_lateralTilt;
			
			/**
				Blade radius (length) in meters.
			*/
			smReal		   m_bladeRadius;

			/**
				Blade chord (width) in meters.
			*/
			smReal			m_bladeChord;

			/**
				Number of blades of the rotor.
			*/
			unsigned int	m_bladeCount;

			/**
				Offset of flapping hinge from the shaft in meters.
			*/
			smReal			m_hingeOffset;

			/**
                Minimum flapping angle in degrees. Blade is not allowed to sink below this level. This is usually a negative number.
            */
			smReal			m_flappingHingeLowerLimit;

			/**
                Maximum flapping angle in degrees. Blade is not allowed to raise above this level. This is usually a positive number.
            */
			smReal			m_flappingHingeHigherLimit;

			/**
				Delta 3 angle in degrees.
			*/
			smReal			m_delta3Angle;

			/**
				Set the parameter to true for a clockwise (when viewed from above) rotating rotor
				and false for a counter clockwise rotating one.
			*/
			bool			m_clockwiseRotation;

			//@}

			/**
				@name Rotor's Mechanical Features
			*/
			//@{

			/**
				Moment of inertia (rotational inertia) of the rotor about the axis of rotation.
				In kg*m^2
			*/
			smReal			m_rotorMomentOfInertia;

			/**
				Moment of inertia (rotational inertia) of a blade about the flapping hinge.
				In kg*m^2.
			*/
			smReal			m_bladeMomentOfInertia;

			/**
                Maximum braking torque (in N*m) that can be applied to the rotor shaft.
            */
			smReal			m_maxBrakeTorque;


			//@}

			/** Swash plate phase angle in degrees. */
			smReal			m_swashPlatePhaseAngle;


			/**
				@name Aerodynamic Features of the Rotor.

			*/
			//@{
		
			/**
				Blade twist in degrees.
			*/
			smReal				m_bladeTwist;

			/**
                This table describes the coefficient of lift. X axis of the table is the Mach number and Y axis is the angle of 
                attack in degrees. 
            */
			RTD::LookUpTable	m_liftCoefficient;

            /**
                This table describes the coefficient of drag. X axis of the table is the Mach number and Y axis is the angle of 
                attack in degrees. 
            */
			RTD::LookUpTable	m_dragCoefficient;

            /** This is an internally used configuration paramater and filled automatically. */
			RTD::LookUpTable	m_liftCurveBreakPntPos1;

            /** This is an internally used configuration paramater and filled automatically. */
			RTD::LookUpTable	m_liftCurveBreakPntPos2;

            /** This is an internally used configuration paramater and filled automatically. */
			RTD::LookUpTable	m_liftCurveBreakPntNeg1;

            /** This is an internally used configuration paramater and filled automatically. */
			RTD::LookUpTable	m_liftCurveBreakPntNeg2;

            /** This parameter is used only in Momentum Theory Compatibility mode. RTDynamics use only. */
			smReal		m_liftCurveSlope;
			
            /** This parameter is used only in Momentum Theory Compatibility mode. RTDynamics use only. */
			smReal		m_liftCurveMax;
			
            /** This parameter is used only in Momentum Theory Compatibility mode. RTDynamics use only. */
			smReal	m_zeroLiftDragCoefficient;


			/**
				Rotors are usually designed for a specific RPM. This parameter contains this
				design RPM for the rotor. The rotor class itself does not use the design RPM
				in calculations. It is here to be used by other classes (for example, GenHeli300
				sets the RPM of attached rotors to their design RPM if user does not attach
				an engine model ).
			*/
			smReal			m_designRPM;


            // todo doc

			smReal		m_interferenceDownwashLowA1;
			smReal		m_interferenceDownwashMediumA1;
			smReal		m_interferenceDownwashHighA1;
			smGraph2D	m_interferenceDownwashCurveLowA1;
			smGraph2D	m_interferenceDownwashCurveMediumA1;
			smGraph2D	m_interferenceDownwashCurveHighA1;

			smReal		m_interferenceInplanewashLowA1;
			smReal		m_interferenceInplanewashMediumA1;
			smReal		m_interferenceInplanewashHighA1;
			smGraph2D	m_interferenceInplanewashCurveLowA1;
			smGraph2D	m_interferenceInplanewashCurveMediumA1;
			smGraph2D	m_interferenceInplanewashCurveHighA1;

			smGraph2D	m_dynamicPressureRatioAoA;
			smGraph2D	m_dynamicPressureRatioSideslip;

            // todo doc
			smReal		m_inducedVelocityThrustFactor;
			smReal		m_inducedVelocityRollMomentFactor;
			smReal		m_inducedVelocityPitchMomentFactor;

			/**
				An empirical paremeter which defines the magnitude of the ground effect. This parameter does not have a unit.
			*/
			smReal			m_groundEffectFactor;
			//@}



            /**
                Enables quasi steady state mode. In this mode, the rotor will be modeled with a single blade which makes one full 
                rotation around the shaft at each simulation step.
                
                Output variables (moments, forces, induced velocity etc. ) will be computed as the average over the full azimuth times 
                the number of blades.

                This parameter is false by default.
            */
			bool			m_quasiSteadyStateModeEnabled;

            /**
                This parameter is used only if m_quasiSteadyStateModeEnabled is true. It determines the number of samples that will
                be computed when the rotor blade is rotated around the shaft. For example, if the m_updateSubstepCount is 180, aerodynamic
                model will be executed for every two degrees.
            */
			unsigned int	m_updateSubstepCount;


            /** Prints the control inputs (theta0, A1 and B1) to the console. */
			bool			m_printControlInputs;

            /** Prints the harmonic flapping coefficients (a0,a1,b1) to the console. */
			bool			m_printFlappingInfo;

            /** Prints the rotor angular speed in rad/sec to the console. */
			bool			m_printOmega;

            /** Prints the forces generated by the rotor to the console. */
			bool			m_printForces;

            /** Prints the moments generated by the rotor to the console. */
			bool			m_printMoments;
            
            /** Prints the induced velocity (m/s) and wake swek angle (deg) to the console. */
			bool			m_printInducedVelocityAndSkewAngle;

            /** This parameter is used to put the rotor model into FGRotorMomentumTheory model compatibility mode. This is
                used by RTDynamics for testing and model validation purposes. */
			bool			m_mtRotorCompatibilityMode;

			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		};

	protected:


		VehicleBase*		m_vehicle;
	public:
		/** Constructor of the class.
		*	\param name				: Name of the rotor.
		*   \param vehicle			: The vehicle to which the rotor is attached.
		*	\param parameters		: Configuration parameters of the rotor.
		*/
		FGRotorBladeElementTheory(	const std::string& name ,
								VehicleBase* vehicle,								
								const ConfigParams& parameters );


		/**
			Destructor of the class.
		*/
		virtual ~FGRotorBladeElementTheory();

		/**
			Creates an instance of FGRotorBladeElementTheory.
			\param attachedVehicle		:	The vehicle to which the ground contact is attached.
			\param name					:   Name of the ground contact instance.
			\param params				:	Configuration parameters of the ground contact.
		*/
		static	FGRotorBladeElementTheory*	createClass( RTD::VehicleBase* attachedVehicle, const std::string& name , const ConfigParams* params  );

		/**
			Creates a ConfigParams structure which can be used to initialize an FGRotorBladeElementTheory instance.
		*/
		static	FGRotorBladeElementTheory::ConfigParams*	createParameters();

		/** Returns FGRotorBladeElementTheory string*/
		virtual std::string		getType() const;

		/**
			Resets the rotor.
		*/
		virtual void	reset();

		/**
			Updates the FGRotorBladeElementTheory by advancing the simulation \e timestep seconds.
		*/
		virtual void update( smTime timestep ) ;




		/** Returns thrust force in Newtons. Thrust is perpendicular to the disc plane (not tip path plane),
			i.e. relative to helicopter its direction is constant. */
		virtual smReal			getT() const;

		/** Drag force of the rotor in Newtons. Drag vector is on the disc plane and positive backwards (towards
			the tail). */
		virtual smReal			getH() const;

		/** Side force of the rotor in Newtons. Side force vector is on the disc plane and positive towards right. */
		virtual smReal			getY() const;

		/**
            Returns the design RPM of the rotor. In case the rotor is not attached to an engine, it will rotate
            with this RPM. When an engine is available in the model and the rotor is connected to it with a drive
            train, RPM of the rotor will not be constant and will be determined with the aerodynamic loads and 
            available engine power.
        */
		virtual smReal			getDesignRPM() const;

		/**
            Returns the rotational inertia (kg*m^2) of the entire rotor around the shaft axis.
        */
		virtual smDouble		getRotationalInertia() const;

        /**
            Returns the actual mean induced velocity in m/s. Induced velocity is not constant at every point on the rotor
            hence this method returns the mean induced velocity.

            Positive induced velocity indicates a downward moving air mass.
        */
		virtual smReal			getInducedVelocity() const;

		/**
            Returns the configuration parameters of this class.
        */
		ConfigParams*			getConfigParams()		{	return dynamic_cast<ConfigParams*>( m_configParams ); }

		/**
            Returns the configuration parameters of this class.
        */
		const ConfigParams*		getConfigParams() const	{	return dynamic_cast<ConfigParams*>( m_configParams ); }

		/**
            This struct defines a vector with a color that is generated by the generateVisualizationData() method.
        */
		struct VisualizationData
		{
            /** \brief Position of the vector in world coordinates. */
			smVec3r m_pos;
            /** \brief Vector in world coordinates. */
			smVec3r m_vec;
            /** \brief Color of the vector in RGBA format where each channel can have a value in [0,1] range. */
			smVec4r m_color;
		};

		/**
            This method is used to generate visualization data that can be used to visualize internal workings
            of the model (e.g. velocities, generated lift and drag force ).

            Visualization data is generated by this method, however no 3D rendering takes place. Rendering of the
            data must be performed by the developers.

            \see VisualizationData
        */
		virtual void			generateVisualizationData( std::vector<VisualizationData>& vizData );
	
	protected:
	
		struct BladeElement;
		struct BladeInfo;
		class RotorInfo;

		RotorInfo*	m_rotorInfo;


		// This matrix transforms a vector/point from fixed shaft axis system to body coordinate system. 
        // This matrix uses mathematical interpretation convention and aviation axis system.
		smMatrix	m_shaft2Body; // constant
		smMatrix	m_shaft2BodyInv;

		// This matrix transforms a vector/point from body axis system to world coordinate system.
		//	This matrix uses mathematical interpretation convention and aviation axis system.
		smMatrix	m_body2WorldNED; // constant
		smMatrix	m_body2WorldInvNED;
 									

		StateVariableDouble			s_psiRad;

		// Harmonic inflow state variables
		StateVariableReal			s_downwashMean;
		StateVariableReal			s_downwashCosine;
		StateVariableReal			s_downwashSine;

		OutputVariableReal			o_inducedVelocity;

		OutputVariableDouble		o_T;
		OutputVariableDouble		o_H;
		OutputVariableDouble		o_Y;		

		OutputVariableDouble		o_Mpitch;
		OutputVariableDouble		o_Mroll;
		OutputVariableDouble		o_Myaw;
			
		OutputVariableReal			o_profilePower;
		OutputVariableReal			o_inducedPower;
		OutputVariableReal			o_parasiteAndClimbPower;

		std::vector<StateVariableReal*>	m_bladeBetaVector;
		std::vector<StateVariableReal*>	m_blade_dBetadt_Vector;

		virtual void    initializeBlades();

		// Computes m_body2Hub matrix
		virtual void	computeStaticMatrices();

		virtual	void 	updateInducedVelocity(	const smVec3r& Vhub_NED_SA , smTime  timestep , smReal& K1x , smReal& K1y );		
		virtual smReal	downwashComponentUpdate( smReal currentValue, smReal targetValue , smTime timestep );

		void	        createAndInitBladeStateVariables( RotorInfo* rotorInfo );
		void	        deleteBladeStateVariables( RotorInfo* rotorInfo );

		virtual	void	printInfo();

	};
}