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
	class FCSHelicopterVelocityController;
	class FCSHelicopterVerticalSpeedController;

	/**
        FCSHelicopterHoverController is a control system component which implements following functions:
        - Hover (HVR)
        - Hover height (H.HT)
        - Transition up (T.UP, transition to forward flight)
        - Transition down (T.DWN, transition to hover )

        Details of the controller can be found in the RotorLib FDM Advanced Autopilot Documentation.
    */
	class RTDYNAMICS_EXPORT FCSHelicopterHoverController :	public RTD::ControlSystemComponentBase
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


	
			int		m_velocityControllerIndex;
			int     m_verticalControllerIndex;

			smReal	m_lateralBeepTrimSpeedKts;
			smReal	m_longitudinalBeepTrimSpeedKts;

			smReal	m_tdownDescendSpeedKIAS;
			smReal	m_tdownDescendRateFtMin;
			
			smReal	m_tdownDescendToHHTStartHeightFeet;
			smReal  m_tdownLateralControlThresholdSpeedKts;
			smReal	m_tdownSwitchToHoverGroundSpeedKts;
			smReal	m_tdownDescendToHHTSpeedControlExponent;
			smReal	m_tdownDescendToHHTMinSpeedHeightFeet;
			smReal	m_tdownDescendToHHTFinalGroundSpeedKts;

			smReal	m_tupInitialSpeedTargetKIAS;
			smReal	m_tupFinalSpeedTargetKIAS;
			smReal	m_tupClimbRateFtMin;
			


			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/** 
			Creates an instance of FCSHelicopterHoverController.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSHelicopterHoverController instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSHelicopterHoverController( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSHelicopterHoverController();


		/** Returns the type of the FCS component. For this class it returns "FCSHelicopterHoverController". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();
	protected:        
		enum TDownPhase
		{
				DESCEND_WITH_CONST_RATE = 0,
				DECELERATE_AND_DESCEND_TO_HHT,
				DECELERATE_TO_HOVER
		};


		ConfigParams	m_parameters;

		RTD::PilotInputData*	getOutputAsPilotInputData();

		virtual void			updateHorizontalHoverControl( RTD::PilotInputData* controlInput );
		virtual void			updateVerticalHoverControl( RTD::PilotInputData* controlInput );
		virtual void			updateHorizontalTDownControl( RTD::PilotInputData* controlInput , TDownPhase phase );
		virtual void			updateVerticalTDownControl( RTD::PilotInputData* controlInput , TDownPhase phase);
		virtual void			updateHorizontalTUpControl( RTD::PilotInputData* controlInput  );
		virtual void			updateVerticalTUpControl( RTD::PilotInputData* controlInput );

		FCSHelicopterVelocityController*			m_slaveVelocityController;
		FCSHelicopterVerticalSpeedController*		m_slaveVerticalController;

	};

}