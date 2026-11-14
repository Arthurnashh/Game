/*
	(c) Copyright 2008, RTDynamics
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
		FGRotorMomentumTheory implements a high fidelity rotor model using momentum theory.

		@see FGRotorMomentumTheory::ConfigParams
	*/
	class RTDYNAMICS_EXPORT FGRotorMomentumTheory : public RotorModelBase
	{
	public:
		/**
			Configuration parameters of the rotor.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RotorModelBase::ConfigParams
		{
		public:

			ConfigParams();

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
				Gear ratio between the engine and the rotor.
				rotor RPM = engine RPM / m_gearRatio

                Note that this parameter will be used only in GenHeli500 model where advanced drive train models
                are not supported. In case a drive train is available in the model, gear ratio is determined by
                the drive train model.
			*/
			smReal			m_gearRatio;
			//@}

			/**
				@name Aerodynamic Features of the Rotor.

			*/
			//@{
		
			/**
				Blade twist in degrees.
			*/
			smReal			m_bladeTwist;


			/**
				Slope of the blade lift curve. X axis of the curve is assumed to be in degrees. This parameter does not have a unit.

				Example: If a lift curve has Clmax = 1.5 at 15 degrees ,m_liftCurveSlope will be 1.5/15 = 0.1.
			*/
			smReal		m_liftCurveSlope;

			/**
				Drag coefficient of the blades at zero lift. This parameter does not have a unit.
			*/
			smReal	m_zeroLiftDragCoefficient;


			/**
				Due to various reasons momentum theory requires a empirical correction factor for the
				computation of the induced power. Following parameter is used for this purpose.
			*/
			smReal		m_inducedPowerCorrectionFactor;

			/**
				This curve contains a coefficient which is used to scale the thrust
				produced by the rotor depending on the translational speed of the rotor in the air.
				It is used fore model fine tuning.

				The curve contains speed in m/s and lift scale as a unitless number.
			*/
			smGraph2D		m_thrustVSSpeedCurve;

			/**
				This curve contains a coefficient which is used to scale the power
				consumed by the rotor depending on the translational speed of the rotor in the air.
				It is used fore model fine tuning.

				The curve contains speed in m/s and power scale as a unitless number.
			*/
			smGraph2D		m_powerVSSpeedCurve;


			smGraph2D		m_flappingVsAdvanceRatio;

			/**
				An empirical paremeter which defines the magnitude of the ground effect. This parameter does not have a unit.
			*/
			smReal			m_groundEffectMagnitude;


			/**
				Rotors are usually designed for a specific RPM. This parameter contains this
				design RPM for the rotor. The rotor class itself does not use the design RPM
				in calculations. It is here to be used by other classes (for example, GenHeli300
				sets the RPM of attached rotors to their design RPM if user does not attach
				an engine model ).
			*/
			smReal			m_designRPM;


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


			bool			m_printControlInputs;
			bool			m_printFlappingInfo;
			bool			m_printOmega;
			bool			m_printForces;
			bool			m_printMoments;
			bool			m_printInducedVelocityAndSkewAngle;
            bool            m_improvedRotorDrag;

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
		FGRotorMomentumTheory(	const std::string& name ,
								VehicleBase* vehicle,								
								const ConfigParams& parameters );


		/**
			Destructor of the class.
		*/
		virtual ~FGRotorMomentumTheory();

		/**
			Creates an instance of FGRotorMomentumTheory.
			\param attachedVehicle		:	The vehicle to which the ground contact is attached.
			\param name					:   Name of the ground contact instance.
			\param params				:	Configuration parameters of the ground contact.
		*/
		static	FGRotorMomentumTheory*	createClass( RTD::VehicleBase* attachedVehicle, const std::string& name , const ConfigParams* params  );

		/**
			Creates a ConfigParams structure which can be used to initialize an FGRotorMomentumTheory instance.
		*/
		static	FGRotorMomentumTheory::ConfigParams*	createParameters();

		/** Returns FGRotorMomentumTheory string*/
		virtual std::string		getType() const;

		/**
			Resets the rotor.
		*/
		virtual void	reset();

		/**
			Updates the FGRotorMomentumTheory by advancing the simulation \e timestep seconds.
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

		/** Returns the blade pitch at given azimuth. Blade pitch is in degrees and is positive when blade 
			angle of attack is positive.
		
			\e psi denotes the azimuth and is in degrees. Azimuth is zero downstream. For a rotor which has 
			its advancing blade on the right side, azimuth is 90 degrees when the blade is on the right.		
		*/
		smReal					getBladePitch( smReal psi );

		/** Returns the blade flapping angle at given azimuth. Blade flapping is in degrees and is positive 
			when blade moves upwards.
		
			\e psi denotes the azimuth and is in degrees. Azimuth is zero downstream. For a rotor which has 
			its advancing blade on the right side, azimuth is 90 degrees when the blade is on the right.		
		*/
		smReal					getBladeFlapping( smReal psi );


		/**
			Gear ratio between the engine and the rotor.
			rotor RPM = engine RPM / m_gearRatio

            \see ConfigParams::m_gearRatio
		*/
		virtual smReal			getGearRatio() const;

		/**
            Returns the design RPM of the rotor. In case the rotor is not attached to an engine, it will rotate
            with this RPM. When an engine is available in the model and the rotor is connected to it with a drive
            train, RPM of the rotor will not be constant and will be determined with the aerodynamic loads and 
            available engine power.
        */
		virtual smReal			getDesignRPM() const;

        /**
            Returns the actual mean induced velocity in m/s. Induced velocity is not constant at every point on the rotor
            hence this method returns the mean induced velocity.

            Positive induced velocity indicates a downward moving air mass.
        */
		virtual smDouble		getRotationalInertia() const;

        /** Returns the actual mean induced velocity in m/s. */
		virtual smReal			getInducedVelocity() const	{	return o_inducedVelocity.getValue();	}

		/**
            Returns the configuration parameters of this class.
        */
		ConfigParams*			getConfigParams()		{	return dynamic_cast<ConfigParams*>( m_configParams ); }

		/**
            Returns the configuration parameters of this class.
        */
		const ConfigParams*		getConfigParams() const	{	return dynamic_cast<ConfigParams*>( m_configParams ); }


	protected:

	



		StateVariableDouble			s_prevVi;
		StateVariableDouble			s_prevT;
		StateVariableDouble			s_prevH;
		StateVariableDouble			s_prevY;
		
		
		StateVariableDouble			s_coning_steady_rad;	// steady state value of coning angle in radians
		StateVariableDouble			s_a1s_steady_rad;	// steady state longitudinal flapping angle in radians, w.r.t shaft (rotor local coordinates)
		StateVariableDouble			s_b1s_steady_rad; //  steady state lateral flapping angle in radians, w.r.t shaft (rotor local coordinates)



		StateVariableDouble			s_lateralTPPAngularRate;
		StateVariableDouble			s_longitudinalTPPAngularRate;
		StateVariableDouble			s_prevGR7;
		StateVariableDouble			s_prevGR8;



		OutputVariableReal			o_profilePower;
		OutputVariableReal			o_inducedVelocity;
		OutputVariableReal			o_inducedPower;
		OutputVariableReal			o_parasiteAndClimbPower;

		OutputVariableDouble		o_T;
		OutputVariableDouble		o_H;
		OutputVariableDouble		o_Y;

		virtual		void	computeRotorForces( const smDouble& Vi,
												const smDouble& Wb,
												const smDouble& p,
												const smDouble& R,
												const smDouble& omega,
												const smDouble& a,
												const smDouble& b,
												const smDouble& c,
												const smDouble& Ua,
												const smDouble& Va,
												const smDouble& Wa,
												const smDouble& A1,
												const smDouble& B1,
												const smDouble& thetaCollective,
												bool computeYH,
												smDouble& T,
												smDouble& H,
												smDouble& Y);

		virtual		smDouble 	inducedVelocityIterationFunction( const smDouble& Vi,
																	const smDouble& Wb,
																	const smDouble& Wr,
																	const smDouble& Ua,
																	const smDouble& Va,
																	const smDouble& Wa,
																	const smDouble& p,
																	const smDouble& R,
																	const smDouble& omega,
																	const smDouble& a,
																	const smDouble& b,
																	const smDouble& c,
																	const smDouble& A1,
																	const smDouble& B1,
																	const smDouble& thetaCollective );

		virtual		bool	secantMethodIterator(	const smDouble& initialVi0,
														const smDouble& initialVi1,
														const smDouble& Wb,
														const smDouble& Wr,
														const smDouble& Ua,
														const smDouble& Va,
														const smDouble& Wa,
														const smDouble& p,
														const smDouble& R,
														const smDouble& omega,
														const smDouble& a,
														const smDouble& b,
														const smDouble& c,
														const smDouble& A1,
														const smDouble& B1,
														const smDouble& thetaCollective,
														smDouble& Vi );

	
		virtual	void 	updateTipPathPlaneDynamicsSteadyState( const smDouble& Vi,
																const smDouble& Wb,
																const smDouble& Wr,
																const smDouble& Ua,
																const smDouble& Va,
																const smDouble& Wa,
																const smDouble& p,
																const smDouble& R,
																const smDouble& omega,
																const smDouble& a,
																const smDouble& b,
																const smDouble& c,
																const smDouble& e,
																const smDouble& T,
																const smDouble& Ib,
																const smDouble& thetaCollective,
																const smDouble& A1,
																const smDouble& B1,
																const smDouble& pitchRate,
																const smDouble& rollRate,
																const smDouble& dt);


		virtual	void 	updateTipPathPlaneDynamicsNonSteadyState( const smDouble& Vi,
																const smDouble& Wb,
																const smDouble& Wr,
																const smDouble& Ua,
																const smDouble& Va,
																const smDouble& Wa,
																const smDouble& p,
																const smDouble& R,
																const smDouble& omega,
																const smDouble& a,
																const smDouble& b,
																const smDouble& c,
																const smDouble& e,
																const smDouble& T,
																const smDouble& Ib,
																const smDouble& A1,
																const smDouble& B1,
																const smDouble& pitchRate,
																const smDouble& rollRate,
																const smDouble& dt);

		virtual void			updateConingAngle(	const smDouble& Vi,
											const smDouble& Ua,
											const smDouble& Va,
											const smDouble& Wa,
											const smDouble& p,
											const smDouble& R,
											const smDouble& omega,
											const smDouble& a,
											const smDouble& c,
											const smDouble& thetaColl,
													const smDouble& B1,
													const smDouble& dt );

		virtual void			updateRotorDynamicsParam( const smDouble& steadyStateValue , const smDouble& omega ,const smDouble& dt, smDouble& param );

		




		virtual	void	printInfo();

	};
}