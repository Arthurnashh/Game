/*
(c) Copyright 2006, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/Common.h"
#include "RTD/ControlSystemComponentBase.h"
#include "RTD/PIDController.h"

namespace RTD
{
	class VehicleBase;

	/**
		This class implements a rotation damper which can be used for stability augmentation in airplanes
		and helicopters. It contains three PID controllers for pitch, yaw and roll control.
	*/
	class RTDYNAMICS_EXPORT FCSRotationDamper :	public RTD::ControlSystemComponentBase
	{
	public:

		/**
			Configuration parameters of the controller.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::ControlSystemComponentBase::ConfigParams
		{
		public:
			ConfigParams();

			/** Proportional coefficient of the yaw controller. */
			smReal				m_yawProportional;

			/** Derivative coefficient of the yaw controller. */
			smReal				m_yawDerivative;

			/** Proportional coefficient of the pitch controller. */
			smReal				m_pitchProportional;

			/** Derivative coefficient of the pitch controller. */
			smReal				m_pitchDerivative;

			/** Proportional coefficient of the roll controller. */
			smReal				m_rollProportional;

			/** Derivative coefficient of the roll controller. */
			smReal				m_rollDerivative;

			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/**
			Creates an instance of FCSRotationDamper.
			\param vehicle		:	The vehicle to which the FCSRotationDamper is attached.
			\param name			:   Name of the FCSRotationDamper instance.
			\param parameters	:	Configuration parameters of the FCSRotationDamper.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* vehicle, const std::string& name , const ConfigParams* parameters  );

		/**
			Creates a ConfigParams structure which can be used to initialize an FCSRotationDamper instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/**
		*	Constructor of the class.
		*   \param parentVh		:	The vehicle to which the FCSRotationDamper
		*          					is attached.
		*	\param name			:   Name of the FCSRotationDamper instance.
		*	\param parameters	:	Configuration parameters of the FCSRotationDamper.
		*/
		FCSRotationDamper( RTD::VehicleBase* parentVh, const std::string& name , const ConfigParams* parameters );

		/**
			Destructor of the class.
		*/
		virtual ~FCSRotationDamper();


		/** Returns "FCSRotationDamper". */
		virtual std::string		getType() const;


		/** Updates the FCS component. To serve as an example source code is provided below:
\code

RTD::PilotInputData* controlOutput  = m_controlOutput;
RTD::PilotInputData* controlInput = NULL;
if ( m_previousStage )
{
	controlInput = m_previousStage->getControlOutput();
}
else
{
	controlInput = m_attachedVehicle->getPilotInputData();
}

// Not entering following loop means that either the input or the output is not available.
// Both of these cases are possibly an indication of a bug.
if  ((controlOutput) && (controlInput))
{
	// First copy the input to output
	m_controlOutput->copy( *controlInput );

	// If enabled go on and compute output values.
	if ( m_enabled )
	{
		smVec3r localRotSpeed = m_attachedVehicle->getState().getRotSpeedBCS();

		smReal pitchOffset = -m_pitchController->update( -localRotSpeed[0] , stepTime );
		controlOutput->m_rollPitchControl[1] =  smMath::clamp( controlInput->m_rollPitchControl[1] + controlInput->m_pitchTrim + pitchOffset , -1.0f , 1.0f );

		smReal rollOffset = m_rollController->update( -localRotSpeed[1] , stepTime );
		controlOutput->m_rollPitchControl[0] =  smMath::clamp( controlInput->m_rollPitchControl[0] + controlInput->m_rollTrim + rollOffset, -1.0f , 1.0f  );

		smReal yawOffset = -m_yawController->update( -localRotSpeed[2] , stepTime );
		controlOutput->m_pedals =  smMath::clamp(	controlInput->m_pedals + controlInput->m_yawTrim + yawOffset, -1.0f , 1.0f  );
	}
}
\endcode


		*/
		virtual void			update( smTime steptime );

		/**Resets the FCS component. */
		virtual void			reset();

		/**
			Returns the configuration parameters of the control system component. 
		*/
		ConfigParams*	getConfigParams()				{	return m_configParams;	}

		/**
			Returns the configuration parameters of the control system component. 
		*/
		const ConfigParams*	getConfigParams() const		{	return m_configParams;	}

	protected:



		RTD::PIDController*	m_pitchController;
		RTD::PIDController*	m_yawController;
		RTD::PIDController*	m_rollController;

		ConfigParams*		m_configParams;

	};

}