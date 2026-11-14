/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/ControlSystemComponentBase.h"

namespace RotorLibFDM
{
	class VehicleBase;

	class RTDYNAMICS_EXPORT FCSGenericControlSystem :	public RTD::ControlSystemComponentBase
	{
	public:

		/** Configuration parameters of the FCS component.	*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::ControlSystemComponentBase::ConfigParams
		{
		public:
			/** Constructor of the class. */
			ConfigParams();

			smReal	m_offsetCollective;
			smReal	m_offsetLateralCyclic;
			smReal	m_offsetLongitudinalCyclic;
			smReal	m_offsetPedal;

			smReal	m_collectiveRightLimit;
			smReal	m_lateralCyclicRightLimit;
			smReal	m_longitudinalCyclicRightLimit;
			smReal	m_pedalRightLimit;

			smReal	m_collectiveLeftLimit;
			smReal	m_lateralCyclicLeftLimit;
			smReal	m_longitudinalCyclicLeftLimit;
			smReal	m_pedalLeftLimit;

			smReal	m_forwardfeedCollective;
			smReal	m_forwardfeedLateralCyclic;
			smReal	m_forwardfeedLongitudinalCyclic;
			smReal	m_forwardfeedPedal;

			smReal	m_crossfeedCollToPedal;	
			smReal	m_crossfeedCollToLateralCyclic;	
			smReal	m_crossfeedCollToLongitudinalCyclic;	
		
			smReal	m_crossfeedPedalToCollective;	
			smReal	m_crossfeedPedalToLateralCyclic;	
			smReal	m_crossfeedPedalToLongitudinalCyclic;	

			smReal	m_crossfeedLatCycToCollective;	
			smReal	m_crossfeedLatCycToLongitudinalCyclic;	
			smReal	m_crossfeedLatCycToPedal;	

			smReal	m_crossfeedLonCycToCollective;	
			smReal	m_crossfeedLonCycToLateralCyclic;	
			smReal	m_crossfeedLonCycToPedal;	

			smReal	m_feedbackPitchRateToCollective;	
			smReal	m_feedbackPitchRateToPedal;	
			smReal	m_feedbackPitchRateToLateralCyclic;	
			smReal	m_feedbackPitchRateToLongitudinalCyclic;	

			smReal	m_feedbackRollRateToCollective;	
			smReal	m_feedbackRollRateToPedal;	
			smReal	m_feedbackRollRateToLateralCyclic;	
			smReal	m_feedbackRollRateToLongitudinalCyclic;	

			smReal	m_feedbackYawRateToCollective;	
			smReal	m_feedbackYawRateToPedal;	
			smReal	m_feedbackYawRateToLateralCyclic;	
			smReal	m_feedbackYawRateToLongitudinalCyclic;	

			smReal	m_feedbackPitchToCollective;	
			smReal	m_feedbackPitchToPedal;	
			smReal	m_feedbackPitchToLateralCyclic;	
			smReal	m_feedbackPitchToLongitudinalCyclic;	

			smReal	m_feedbackRollToCollective;	
			smReal	m_feedbackRollToPedal;	
			smReal	m_feedbackRollToLateralCyclic;	
			smReal	m_feedbackRollToLongitudinalCyclic;	

			smReal	m_feedbackYawToCollective;	
			smReal	m_feedbackYawToPedal;	
			smReal	m_feedbackYawToLateralCyclic;	
			smReal	m_feedbackYawToLongitudinalCyclic;	

			smReal	m_collectiveAttitudeFeedbackAuthorithyHigh;	
			smReal	m_collectiveAttitudeFeedbackAuthorithyLow;	

			smReal	m_lateralCyclicAttitudeFeedbackAuthorithyHigh;	
			smReal	m_lateralCyclicAttitudeFeedbackAuthorithyLow;	

			smReal	m_longitudinalCyclicAttitudeFeedbackAuthorithyHigh;	
			smReal	m_longitudinalCyclicAttitudeFeedbackAuthorithyLow;	

			smReal	m_pedalAttitudeFeedbackAuthorithyHigh;	
			smReal	m_pedalAttitudeFeedbackAuthorithyLow;	

			smReal	m_pedalAttitudeFeedbackSpeedLimitLow;
			smReal	m_pedalAttitudeFeedbackSpeedLimitHigh;

			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/** 
			Creates an instance of FCSGenericControlSystem.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSGenericControlSystem instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSGenericControlSystem( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSGenericControlSystem();


		/** Returns the type of the FCS component. For this class it returns "FCSGenericControlSystem". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();

		virtual void			enableFeedbackLoops( bool en )	{  s_areFeedbackLoopsEnabled = en;	}
		bool					areFeedbackLoopsEnabled() const	{	return s_areFeedbackLoopsEnabled;}

	protected:        

		virtual smReal computeControlOutput(	smReal offset, smReal fromCollective, smReal fromLatCyc , smReal fromLonCyc , smReal fromPedal,
												smReal fromPitch, smReal fromRoll , smReal fromYaw,
												smReal fromPitchRate, smReal fromRollRate , smReal fromYawRate,
												smReal attitudeFeedbackAuthorithyLow , smReal attitudeFeedbackAuthorithyHigh,
												smReal leftLimit,smReal rightLimit,
												smReal collective,smReal  latcyc, smReal  loncyc,smReal  pedals,
												const smVec3r& desiredHPR,
												smReal attitudeFeedbackSpeedLimitLow,smReal attitudeFeedbackSpeedLimitHigh,
												bool mapToZeroOne );

		ConfigParams	m_parameters;

		RTD::StateVariableReal		s_currentTrimHeading;
		RTD::StateVariableBool		s_areFeedbackLoopsEnabled;
		RTD::StateVariableBool		s_trimHeadingNotInitialised;
		

		RTD::PilotInputData*	getOutputAsPilotInputData();
	};

}