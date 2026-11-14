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
	class FCSHelicopterAttitudeController;

	/**
        FCSHelicopterHeadingController is a control system component which implements heading acquire and hold
        ( a.k.a. HDG mode) functions.

        Details of the controller can be found in the RotorLib FDM Advanced Autopilot Documentation.        
    */
	class RTDYNAMICS_EXPORT FCSHelicopterHeadingController :	public RTD::ControlSystemComponentBase
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

			unsigned int				m_attitudeControllerIndex;

			smReal						m_headingErrorThresholdForBanking;
			smReal						m_maxBankAngle;

			smReal						m_coordinatedTurnThresholdAngle;
			smReal						m_coordinatedTurnThresholdSpeedKIAS;
			smReal						m_bankToSpeedRatioDegPerKIAS;

			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};



		/** 
			Creates an instance of FCSHelicopterHeadingController.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSHelicopterHeadingController instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSHelicopterHeadingController( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSHelicopterHeadingController();


		/** Returns the type of the FCS component. For this class it returns "FCSHelicopterHeadingController". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();

		/**            
            Normally, desired heading is updated automatically depending on pilot inputs according the the underlying
            control law. However, if the controller is put to slave mode, this method can be used to set the desired 
            heading directly.

            \see enableSlaveMode()

            \param dh Desired heading in degrees.

        */
		virtual void			setDesiredHeading( smReal dh );

		/**
            Returns the heading which will be acquired and hold by the controller. Returned value is in degrees.
        */
		smReal					getDesiredHeading() const;


		/** Returns the configuration parameters of this controller. */
		ConfigParams&			getConfigParams();

		/** Returns the configuration parameters of this controller. */
		const ConfigParams&		getConfigParams() const;
		

	protected:        
        RTD::PilotInputData*	getOutputAsPilotInputData();

		ConfigParams	m_parameters;

		RTD::StateVariableReal	s_desiredHeading;
		
		FCSHelicopterAttitudeController*	m_attitudeController;
				


	};

}