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
        FCSHelicopterNavigationController is a control system component which implements autopilot 
        navigation mode (NAV).

        Details of the controller can be found in the RotorLib FDM Advanced Autopilot Documentation.
    */
	class RTDYNAMICS_EXPORT FCSHelicopterNavigationController :	public RTD::ControlSystemComponentBase
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

			smReal						m_bankToSpeedRatioDegPerKIAS;
			smReal						m_maxBankAngle;

			smReal						m_coordinatedTurnThresholdSpeedKIAS;


			smReal						m_maxRouteApproachAngle;
			smReal						m_approachModeAngleFactor;			
			smReal						m_trackModeAngleFactor;
			smReal						m_trackModeStartAngle;
			smReal						m_trackModeMaxAngle;

			smReal						m_coordinatedTurnAuthorityChangeRate;

			smReal						m_waypointAdvancementFactor;

			RTD::PIDController			m_rollControlPIDParams;

			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/** 
			Creates an instance of FCSHelicopterNavigationController.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSHelicopterNavigationController instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSHelicopterNavigationController( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSHelicopterNavigationController();


		/** Returns the type of the FCS component. For this class it returns "FCSHelicopterNavigationController". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();

	protected:        
		
		ConfigParams	m_parameters;

		RTD::PilotInputData*	getOutputAsPilotInputData();

		FCSHelicopterAttitudeController*		m_slaveAttitudeController;



		virtual void	parseRouteString( const std::string& route ) ;

		struct Waypoint
		{
			smDouble	x;
			smDouble	y;

		};

		std::vector<Waypoint>	m_waypoints;

		std::string m_currentRouteString;

		virtual void	updateControlOutputs( const RTD::PilotInputData* pid , smTime stepTime );

		RTD::PIDController*	mpid_bankController;

		RTD::OutputVariableReal o_XTK;
		RTD::OutputVariableReal o_TKE;

	};

}