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
        FCSHelicopterHoverController is a control system component which implements lateral and longitudinal speed
        acquire and hold functions.

        Details of the controller can be found in the RotorLib FDM Advanced Autopilot Documentation.
    */
	class RTDYNAMICS_EXPORT FCSHelicopterVelocityController :	public RTD::ControlSystemComponentBase
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

			smReal				m_pitchPIDProportional;
			smReal				m_pitchPIDDerivative;
			smReal				m_pitchPIDIntegral;
			smReal				m_pitchPIDIntegralMax;
			smReal				m_pitchPIDIntegralMin;
			smReal				m_pitchPIDOffset;
			smReal				m_pitchPIDMaxOutput;
			smReal				m_pitchPIDMinOutput;

			smReal				m_rollPIDProportional;
			smReal				m_rollPIDDerivative;
			smReal				m_rollPIDIntegral;
			smReal				m_rollPIDIntegralMax;
			smReal				m_rollPIDIntegralMin;
			smReal				m_rollPIDOffset;
			smReal				m_rollPIDMaxOutput;
			smReal				m_rollPIDMinOutput;


			smReal				m_beepTrimSpeedIncrementKtsPerSec;

			bool				m_printPitchControllerState;
			bool				m_printRollControllerState;

			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};


        /** */
		enum SpeedMeasurementType
		{
			GROUND_SPEED = 0,
				INDICATED_SPEED
		};

		/** 
			Creates an instance of FCSHelicopterVelocityController.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSHelicopterVelocityController instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSHelicopterVelocityController( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSHelicopterVelocityController();


		/** Returns the type of the FCS component. For this class it returns "FCSHelicopterVelocityController". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();
		
        /** @name Slave Mode Operation

            Following methods are used in slave mode operation to set and query controller inputs externally.

            Note that for slave mode operation, enableSlaveMode(true) must be called at every simulation step. Otherwise,
            the controller will override the externally set inputs.
        */
        //@{
        
        /** \brief Set the desired longitudinal speed in knots. */
		virtual void			setDesiredLongitudinalSpeedKts( smReal speedKts );

		/** \brief Set the measurement method (e.g. measure ground speed or indicated air speed ) for the desired longitudinal speed.  */
		virtual void			setLongitudinalSpeedMeasurementType( SpeedMeasurementType smt );

		/** \brief Return actual desired longitudinal speed. */
		smReal					getDesiredLongitudinalSpeedKts() const;

		/** \brief Enable/disable longitundal control. No longitudinal control action will take place when longitudinal control is disabled. */
		virtual void			enableLongitudinalControl( bool en );

		/** \brief Returns true if longitudinal control is enabled. */
		bool					isLongitudinalControlEnabled() const;

		/** \brief Set the desired lateral speed in knots. */
		virtual void			setDesiredLateralSpeedKts( smReal speedKts );

        /** \brief Set the measurement method (e.g. measure ground speed or indicated air speed) for the desired lateral speed.  */
		SpeedMeasurementType	getLongitudinalSpeedMeasurementType() const;

		/** \brief Return actual desired lateral speed. */
		smReal					getDesiredLateralSpeedKts() const;

		/** \brief Enable/disable longitundal control. No lateral control action will take place when lateral control is disabled. */
		virtual void			enableLateralControl( bool en );

		/** \brief Returns true if lateral control is enabled. */
		bool					isLateralControlEnabled() const;
        //@}

		


	protected:        
		ConfigParams	m_parameters;

		RTD::PilotInputData*	getOutputAsPilotInputData();

		RTD::PIDController*		m_pitchController;
		RTD::PIDController*		m_rollController;

		RTD::StateVariableReal	s_longitudinalSpeedKts;
		RTD::StateVariableBool	s_activateLongitudinalControl;
		RTD::StateVariableBool	s_activateLateralControl;
		RTD::StateVariableReal	s_desiredLateralSpeedKts;
		RTD::StateVariableUInt	s_longitudinalSpeedMeasurementType;

		FCSHelicopterAttitudeController*	m_slaveAttitudeController;
	};

}