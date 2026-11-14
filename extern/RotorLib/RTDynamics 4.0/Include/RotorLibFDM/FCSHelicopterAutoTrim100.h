/*
(c) Copyright 2006, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/ControlSystemComponentBase.h"
#include "RTD/PIDController.h"

namespace RotorLibFDM
{
	class VehicleBase;

	/** 
		This control system component is used to trim the helicopter automatically and hence remove the 
		coupling between controls (i.e. collective / pedal coupling). It translates pilot inputs to requested heading, 
		pitch and roll rates or to requested sideslip and controls the actuators to achieve the desired state. The final 
		result is a simpler flight.

		FCSHelicopterAutoTrim100 is suitable for simulators where simple input devices, such as a joystick, is used. 
		In case real helicopter input devices with trim lock controls are used, FCSHelicopterAutoTrim100
		shouldn't be used.

	*/
	class RTDYNAMICS_EXPORT FCSHelicopterAutoTrim100 :	public RTD::ControlSystemComponentBase
	{
	public:

		/** Configuration parameters of the FCS component.	*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::ControlSystemComponentBase::ConfigParams
		{
		public:
			/** Constructor of the class. */
			ConfigParams();

			/** 
				@name Sideslip PID controller parameters
				
				This PID controller is used to attain a requested sideslip at high speeds. 
				At low speeds, Yaw rate controller is in charge.

				Following parameters are used by this PID controller.

			*/
			//@{
			smReal				m_sideslipPIDProportional;
			smReal				m_sideslipPIDDerivative;
			smReal				m_sideslipPIDIntegral;
			smReal				m_sideslipPIDIntegralMax;
			smReal				m_sideslipPIDIntegralMin;
			smReal				m_sideslipPIDOffset;
			//@}

            /** This parameter defines the maximum side slip angle that can be requested by the
             *  pilot. When pedals are fully pressed (in either direction) control system
             *  will try to reach this side slip. The parameter is in degrees.
             **/
			smReal				m_maxRequestedSideslip;


			/** 
				@name Yaw rate PID controller parameters
				
				This PID controller is used to attain requested yaw rate a low speeds. At
				high speeds, the Side slip controller is in charge.
				
				Following parameters are used by this PID controller.

			*/
			//@{
			smReal				m_yawRatePIDProportional;
			smReal				m_yawRatePIDDerivative;
			smReal				m_yawRatePIDIntegral;
			smReal				m_yawRatePIDIntegralMax;
			smReal				m_yawRatePIDIntegralMin;
			smReal				m_yawRatePIDOffset;
			//@}


            /** This parameter defines the maximum yaw rate that can be requested by the
             *  pilot. When pedals are fully pressed (in either direction) control system
             *  will try to reach this yaw rate. The parameter is in deg/sec.
             **/
			smReal					m_maxRequestedYawRate;
			
			/**
				In some cases it might be better to map the pilot input to the control system
				non-linearly. For example, in a desktop simulator, where the input device is a
				common joystick, we might want to have a better precision at the zero point.

				The actual input that is given to the control system is computed as follows:

				(|pedal input| ^ m_yawControlNonlinearity) * sign( pedal input )

				m_yawControlNonlinearity is by default 1.0 , which is linear mapping.
			*/
			float					m_yawControlNonlinearity;


			/**
				@name Transition from Yaw Rate mode to Side Slip mode

				The controller is designed such that it interprets the pilot pedal input as 
				desired yaw rate at low speeds and as desired side slip at high speeds. A transition region based 
				on speed can be defined to enable a smooth transition between these modes .

				Below m_yawRateToSideslipTransitionSpeedLow the pedal input will be interpreted as desired yaw rate 
				and after m_yawRateToSideslipTransitionSpeedHigh as desired side slip. Between these two speeds, the 
				outputs of the Yaw Rate and Sideslip subcontrollers will be linearly mixed.
			*/
			//@{
			smReal					m_yawRateToSideslipTransitionSpeedLow;
			smReal					m_yawRateToSideslipTransitionSpeedHigh;
			//@}

			/** 
				@name Pitch rate PID controller parameters
				
				A PID controller is used to attain requested pitch rate. Following
				parameters are used by this PID controller.

			*/
			//@{
			smReal				m_pitchRatePIDProportional;
			smReal				m_pitchRatePIDDerivative;
			smReal				m_pitchRatePIDIntegral;
			smReal				m_pitchRatePIDIntegralMax;
			smReal				m_pitchRatePIDIntegralMin;
			smReal				m_pitchRatePIDOffset;
			//@}

            /** This parameter defines the maximum pitch rate that can be requested by the pilot.
            *   When longitudinal cyclic is moved fully back or front control system will try to
            *   reach this pitch rate. The parameter is in deg/sec.
            **/
			smReal					m_maxRequestedPitchRate;

			/**
				In some cases it might be better to map the pilot input to the control system
				non-linearly. For example, in a desktop simulator, where the input device is a
				common joystick, we might want to have a better precision at the zero point.

				The actual input that is given to the control system is computed as follows:

				(|longitudinal cyclic input| ^ m_pitchControlNonlinearity) * sign( longitudinal cyclic input )

				m_pitchControlNonlinearity is by default 1.0 , which is linear mapping.
			*/
			float					m_pitchControlNonlinearity;

			/** 
				@name Roll rate PID controller parameters
				
				A PID controller is used to attain requested roll rate. Following
				parameters are used by this PID controller.

			*/
			//@{
			smReal				m_rollRatePIDProportional;
			smReal				m_rollRatePIDDerivative;
			smReal				m_rollRatePIDIntegral;
			smReal				m_rollRatePIDIntegralMax;
			smReal				m_rollRatePIDIntegralMin;
			smReal				m_rollRatePIDOffset;
			//@}

            /** This parameter defines the maximum roll rate that can be requested by
            *   the pilot. When lateral cyclic is moved fully left or right control
            *   system will try to reach this roll rate. The parameter is in deg/sec.
            **/
			smReal					m_maxRequestedRollRate;

			/**
				In some cases it might be better to map the pilot input to the control system
				non-linearly. For example, in a desktop simulator, where the input device is a
				common joystick, we might want to have a better precision at the zero point.

				The actual input that is given to the control system is computed as follows:

				(|lateral cyclic input| ^ m_rollControlNonlinearity) * sign( lateral cyclic input )

				m_rollControlNonlinearity is by default 1.0 , which is linear mapping.
			*/
			float					m_rollControlNonlinearity;


            /** If set to true yaw rate controller will print its status to console. The
             *  output will be in SMNOT_INFO notification level. */
			bool					m_yawControllerDebugOutput;

            /** If set to true pitch rate controller will print its status to console. The
             *  output will be in SMNOT_INFO notification level. */
			bool					m_pitchControllerDebugOutput;

            /** If set to true roll rate controller will print its status to console. The
             *  output will be in SMNOT_INFO notification level.
             *  */
			bool					m_rollControllerDebugOutput;


			/** 
            *   @name Controller states
            *  
            *   Each controller (also known as channels in flight control system literature )
            *   can be individually turned on and off with these flags.
            *  
            *   They are all true by default.
			*/
			//@{
            bool                    m_yawAndSideslipControllerEnabled;
            bool                    m_pitchControllerEnabled;
            bool                    m_rollControllerEnabled;
			//@}            

			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/** 
			Creates an instance of FCSHelicopterAutoTrim100.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSHelicopterAutoTrim100 instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSHelicopterAutoTrim100( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSHelicopterAutoTrim100();


		/** Returns the type of the FCS component. For this class it returns "FCSHelicopterAutoTrim100". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();
	protected:        
		ConfigParams	m_parameters;

		virtual void	updateYawRateAndSideslipControl(RTD::PilotInputData* controlInput,smTime stepTime);
		virtual void	updatePitchRateControl(RTD::PilotInputData* controlInput,smTime stepTime);
		virtual void	updateRollRateControl(RTD::PilotInputData* controlInput,smTime stepTime);
		virtual smReal	mapInput( smReal inp , smReal power );

		RTD::PilotInputData*	getOutputAsPilotInputData();

		RTD::PIDController*	m_sideslipController;
		RTD::PIDController*	m_yawRateController;
		RTD::PIDController*	m_pitchRateController;
		RTD::PIDController*	m_rollRateController;
	};

}