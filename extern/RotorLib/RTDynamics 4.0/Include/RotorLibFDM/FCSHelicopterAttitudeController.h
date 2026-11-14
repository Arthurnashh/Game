/*
(c) Copyright 2011, RTDynamics
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
        FCSHelicopterAttitudeController is a control system component which implements following functions:
        - Pitch, roll and yaw damping
        - Pitch, roll and heading hold
        - Directional stabilization during coordinated turn

        Details of the controller can be found in the RotorLib FDM Advanced Autopilot Documentation.
    */
	class RTDYNAMICS_EXPORT FCSHelicopterAttitudeController :	public RTD::ControlSystemComponentBase
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

            //  Beep trim parameters
			smReal						m_beepTrimLateralEffectiveness;
			smReal						m_beepTrimLongitudinalEffectiveness;

            // Pitch control
			smReal						m_pitchP;
			smReal						m_pitchI;
			smReal						m_pitchD;
			smReal						m_pitchPClamp;
			smReal						m_pitchPturbulence;
			smReal						m_pitchIturbulence;
			smReal						m_pitchDturbulence;
			smReal						m_pitchPClampturbulence;

			smReal						m_longitudinalCyclicAuthorityPercent;
			smReal						m_pitchBeepTrimEffectiveness;

			// Roll control
			smReal						m_rollP;
			smReal						m_rollI;
			smReal						m_rollD;
			smReal						m_rollPClamp;
			smReal						m_rollPturbulence;
			smReal						m_rollIturbulence;
			smReal						m_rollDturbulence;
			smReal						m_rollPClampturbulence;
			smReal						m_lateralCyclicAuthorityPercent;
			smReal						m_rollBeepTrimEffectiveness;

			// Heading/yaw control
			smReal						m_yawP;
			smReal						m_yawI;
			smReal						m_yawD;
			smReal						m_yawPClamp;
			smReal						m_yawPturbulence;
			smReal						m_yawIturbulence;
			smReal						m_yawDturbulence;
			smReal						m_yawPClampturbulence;
			smReal						m_pedalAuthorityPercent;
			smReal						m_maxYawRateForHeadingHold;

            // Coordinated turn
			smReal						m_coordinatedTurnMinSpeedKIAS;
			smReal						m_coordinatedTurnMinBankAngle;
			smReal						m_coordinatedTurnTransitionSpeed;
			smReal						m_coordinatedTurnP;
			smReal						m_coordinatedTurnPturbulence;


            // Debugging functions
			bool						m_printPitchControl;
			bool						m_printRollControl;
			bool						m_printYawControl;

			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};



		/** 
			Creates an instance of FCSHelicopterAttitudeController.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSHelicopterAttitudeController instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSHelicopterAttitudeController( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSHelicopterAttitudeController();


		/** Returns the type of the FCS component. For this class it returns "FCSHelicopterAttitudeController". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();

		/**                
            @name Driving the controller externally 
        
            Following methods are used to drive the pitch,roll and yaw channels of the controller from another controller. 
            Normally, desired attitude and heading are updated automatically by the internal control law, however, this 
            automatic update will not take place for a channel for one simulation step if corresponding set method is called. 
            This means that, developers must call these methods continuously (even if the values do not change) to use the 
            attitude controller as a slave controller.

            Note that one simulation step is not necessarly a single RTD::World::update() call. A world update
            might result in multiple simulation steps depending on the time passed since previous update.

            
        */

        //@{
        /** \param  dp  Desired pitch angle in degrees. */
		virtual void			setDesiredPitch( smReal dp );
        /** \param  dh  Desired pitch heading in degrees. */
        virtual void			setDesiredHeading( smReal dh );

        /** \param  dr  Desired roll angle in degrees. */
        virtual void			setDesiredRoll( smReal dr );

        /** \param  cta  Desired coordinated turn support authority ratio. Normalized in [0..1] range. */
        virtual void			setCoordTurnAuthority( smReal cta );
        //@}

		/** Returns the current desired pitch in degrees. */
		smReal					getDesiredPitch() const;

		
		/** Returns the current desired roll in degrees. */
		smReal					getDesiredRoll() const;

        /** Returns the current desired heading in degrees. */
        smReal					getDesiredHeading() const;


        /** Returns current authority percentage of the coordinated turn support circuit.
            The returned values is in [0..1] range, where 0 means no authority at all and
            1 means maximum authority.
        */
		smReal					getCoordTurnAuthority() const;


		/** Returns the configuration parameters of this controller. */
		ConfigParams&			getConfigParams();

		/** Returns the configuration parameters of this controller. */
		const ConfigParams&		getConfigParams() const;

		

	protected:        
        RTD::PilotInputData*	getOutputAsPilotInputData();

		ConfigParams	m_parameters;

		RTD::StateVariableReal	s_desiredPitch;
		RTD::StateVariableReal	s_desiredRoll;
		RTD::StateVariableReal	s_desiredHeading;

		bool					m_useExternalCoordTurnAuth;
		bool					m_useExternalDesiredPitch;
		bool					m_useExternalDesiredRoll;
		bool					m_useExternalDesiredHeading;

		RTD::StateVariableReal	s_coordturnAuthority;

		RTD::StateVariableRealDataHistory	s_headingHistory;
		RTD::StateVariableRealDataHistory	s_sideslipHistory;
		RTD::StateVariableRealDataHistory	s_yawRateHistory;
	};

}