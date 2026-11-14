/*
(c) Copyright 2006, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "FixedWingLibFDM/Common.h"
#include "RTD/ControlSystemComponentBase.h"
#include "RTD/PIDController.h"

namespace FixedWingLibFDM
{
	class VehicleBase;
	/** 		
		This class implements an auto-trim control system for airplanes. Current implementation
		controls only the elevators.

		FCSAirplaneAutoTrim100 interprets pilot's longitudinal stick input as requested pitch rate
		(rather than one-to-one translating this input to elevator movement) and controls the elevators
		to have such a pitch rate.

		FCSAirplaneAutoTrim100 also makes angle of attack and load limiting. Even though pilot's 
		request exceeds	the limits the controller keeps the airplane within safe limits.
	*/
	class RTDYNAMICS_EXPORT FCSAirplaneAutoTrim100 :	public RTD::ControlSystemComponentBase
	{
	public:

		/** Configuration parameters of the FCS component.	*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::ControlSystemComponentBase::ConfigParams
		{
		public:
			/** Constructor of the class. */
			ConfigParams();

			/** 
				The final control equation receives pilot input, current load and current angle of
				attack as inputs multiplied by weight factor. This parameter is the weight factor
				of load factor input. The higher is the weight the less load will be allowed by the
				FCS.
			*/
			smReal				m_pitchRateLoadWeight;

			/** 
				@name Angle of attack limiter

				These parameters define the behaviour of the angle of attack limiter.  If the angle of 
				attack is higher than \e m_AoALimiterStartAngleHigh or lower than \e m_AoALimiterStartAngleLow 
				AoA limiter is activated. 
				
				The difference between current AoA and the limit (m_AoALimiterStartAngleHigh or 
				m_AoALimiterStartAngleLow ) multiplied by \e m_pitchRateAoAWeight determines the
				amount of corrective control input to be applied to the elevators to bring the AoA
				within the limits.

				\e m_AoALimiterStartAngleHigh and \e m_AoALimiterStartAngleLow are in degrees. 
				\e m_pitchRateAoAWeight is a unitless control factor.
			*/
			//@{
			smReal				m_pitchRateAoAWeight;
			smReal				m_AoALimiterStartAngleHigh;
			smReal				m_AoALimiterStartAngleLow;
			//@}
			

			/** 			
				The final control equation receives pilot input, current load and current angle of
				attack as inputs multiplied by weight factor. This parameter is the weight factor
				of the pilot's input. The higher is the weight the higher will be the requested pitch rate
				for unit stick movement.
			*/
			smReal				m_pitchRatePilotInputWeight;

			/** 
				@name Pitch rate PID controller parameters
				
				A PID controller is used to attain requested pitch rate. Following
				parameters are used by this PID controller.

			*/
			//@{
			smReal				m_pitchPIDProportional;
			smReal				m_pitchPIDDerivative;
			smReal				m_pitchPIDIntegral;
			smReal				m_pitchPIDIntegralMax;
			smReal				m_pitchPIDIntegralMin;
			smReal				m_pitchPIDOffset;
			//@}

			/** 
				The elevator movement is attenuated with speed after a certain speed threshold.
				This prevents instabilities at higher speeds. This parameter determines the 
				speed starting from which the attenuation will be applied. It is in m/s.
			*/
			smReal				m_pitchSpeedAttenuationStartSpeed;


			/** 
				In some cases it might be useful if during take off and touch down autotrim system
				is not in control of the aircraft. For example, if autotrim is enabled at the moment
				of touch down, even if engines are at idle, the airplane nose will not go down because
				the control system  will try to keep it at its current position. Only after enough speed 
				is lost, or if the pilot commands it, nose will fall. 
				
				If this behaviour is not wanted this parameter can be used to reduce the authorithy
				of the control system in case landing gears are in contact with the ground. When gears
				lose contact with the ground authorithy increases to 100% and when they touch the ground
				authority decreases to 0%. The speed at which the rise/decline happens can be controlled
				with \e m_authorityRatioChangeFactor.

				Setting \e m_authorityRatioChangeFactor to a negative value disables the feature. In this
				case auto-trim controller is always active.
			*/
			smReal				m_authorityRatioChangeFactor;

			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/** 
			Creates an instance of FCSAirplaneAutoTrim100.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSAirplaneAutoTrim100 instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSAirplaneAutoTrim100( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSAirplaneAutoTrim100();


		/** Returns the type of the FCS component. For this class it returns "FCSAirplaneAutoTrim100". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();
	protected:
		RTD::StateVariableReal		s_authorityRatio;
		ConfigParams				m_parameters;

		RTD::PIDController*	m_pitchController;
	};

}