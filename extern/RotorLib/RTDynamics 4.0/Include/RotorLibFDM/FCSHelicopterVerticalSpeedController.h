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
        FCSHelicopterVerticalSpeedController is a control system component which implements following autopilot functions
        - Altitude hold  (ALT)
        - Altitude acquire and hold (ALT.A)
        - Vertical speed hold ( V/S )
        
        Details of the controller can be found in the RotorLib FDM Advanced Autopilot Documentation.
    */
	class RTDYNAMICS_EXPORT FCSHelicopterVerticalSpeedController :	public RTD::ControlSystemComponentBase
	{
	public:

        /** This class contains the configuration parameters of this control system component. 
		
			These parameters are explained in the RotorLib FDM Advanced Autopilot Documentation. 
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::ControlSystemComponentBase::ConfigParams
		{
		public:
			/** Constructor of the class. */
			ConfigParams();

			unsigned int		m_attitudeControllerIndex;

			smReal				m_vertSpeedPIDProportional;
			smReal				m_vertSpeedPIDDerivative;
			smReal				m_vertSpeedPIDIntegral;
			smReal				m_vertSpeedPIDIntegralMax;
			smReal				m_vertSpeedPIDIntegralMin;
			smReal				m_vertSpeedPIDOffset;
			
			smReal				m_pitchPIDProportional;
			smReal				m_pitchPIDDerivative;
			smReal				m_pitchPIDIntegral;
			smReal				m_pitchPIDIntegralMax;
			smReal				m_pitchPIDIntegralMin;
			smReal				m_pitchPIDOffset;
			smReal				m_pitchPIDMaxOutput;
			smReal				m_pitchPIDMinOutput;

			smReal				m_collectivePIDProportional;
			smReal				m_collectivePIDDerivative;
			smReal				m_collectivePIDIntegral;
			smReal				m_collectivePIDIntegralMax;
			smReal				m_collectivePIDIntegralMin;
			smReal				m_collectivePIDProportionalMin;
			smReal				m_collectivePIDProportionalMax;
			smReal				m_collectivePIDOffset;

			bool				m_printCollectiveControllerState;
			bool				m_printPitchControllerState;
			bool				m_printVertSpeedControllerState;

			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/** 
			Creates an instance of FCSHelicopterVerticalSpeedController.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSHelicopterVerticalSpeedController instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSHelicopterVerticalSpeedController( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSHelicopterVerticalSpeedController();


		/** Returns the type of the FCS component. For this class it returns "FCSHelicopterVerticalSpeedController". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();


		/** Returns the desired altitude for ALT mode. */
		smReal					getALTDesiredAltitudeFeet() const ;

		/** Returns the desired altitude for ALT.A mode. */
		smReal					getALTADesiredAltitudeFeet() const ;

		/**
            This methods is used in slave mode operation to set controller inputs externally.

            Note that for slave mode operation, enableSlaveMode(true) must be called at every simulation step. Otherwise,
            the controller will override the externally set inputs.            

            For pure vertical speed control, \e enableAltControl must be false and either \e enableVerticalSpeedControlByPitch or 
            \e enableVerticalSpeedControlByCollective must be true.

            For altitude control, \e enableAltControl must be true and either \e enableVerticalSpeedControlByPitch or 
            \e enableVerticalSpeedControlByCollective must be true.

            \param enableAltControl Enables altitude control. When false, \e desiredAltitudeFeet argument is ignored. 

            \param desiredAltitudeFeet Altitude to acquire and hold if the altitude control ( \e enableAltControl ) is enabled.

            \param enableVerticalSpeedControlByPitch When true, vertical speed (and indirectly altitude) control is performed by controlling the pitch attitude.

            \param enableVerticalSpeedControlByCollective When true, vertical speed (and indirectly altitude) control is performed by controlling the collective.

            \param verticalSpeedFpmin Desired vertical speed in feets per minute. In case altitude control is enabled, abs(verticalSpeedFpmin) is used as a limit 
                    value for the automatically computed vertical speed.
        */
		virtual void			configure(	bool enableAltControl , 
											smReal desiredAltitudeFeet,
											bool enableVerticalSpeedControlByPitch,
											bool enableVerticalSpeedControlByCollective,
											smReal verticalSpeedFpmin);

		/**
            Returns the current configuration of the controller.

            \see configure()
        */
		virtual void			getConfiguration(	bool& enableAltControl , 
													smReal& desiredAltitudeFeet,
													bool& enableVerticalSpeedControlByPitch,
													bool& enableVerticalSpeedControlByCollective,
													smReal& verticalSpeedFpmin);
	protected:        
		ConfigParams	m_parameters;

		RTD::PilotInputData*	getOutputAsPilotInputData();

		RTD::PIDController*	m_pitchController;
		RTD::PIDController*	m_verticalSpeedController;

		RTD::StateVariableReal	s_desiredAltitudeALTFeet;
		RTD::StateVariableReal	s_desiredAltitudeALTAFeet;
		RTD::StateVariableReal	s_desiredVerticalSpeedFtMin;

		RTD::StateVariableBool	s_enableAltitudeControl;
		RTD::StateVariableBool	s_enableVerticalSpeedControlByPitch;
		RTD::StateVariableBool	s_enableVerticalSpeedControlByCollective;
		RTD::StateVariableReal	s_desiredAltitudeFeet;

		FCSHelicopterAttitudeController*	m_slaveAttitudeController;
	};

}