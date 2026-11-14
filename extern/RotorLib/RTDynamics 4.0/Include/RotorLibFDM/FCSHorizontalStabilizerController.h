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

	/** 
		This class implements a control system similar to UH-60's horizontal stabilizer controller.
	*/
	class RTDYNAMICS_EXPORT FCSHorizontalStabilizerController :	public RTD::ControlSystemComponentBase
	{
	public:

		/** Configuration parameters of the FCS component.	*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::ControlSystemComponentBase::ConfigParams
		{
		public:
			/** Constructor of the class. */
			ConfigParams();

			/** Incidence angle of the horizontal stabilizer in degrees vs speed of the helicopter in knots.
				Control system will change the incidence angle of the stabilizer according to this curve. */
			smGraph2D	m_incidenceVsSpeed;

			/** 
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/** 
			Creates an instance of FCSHorizontalStabilizerController.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params  );

		/** 
			Creates a ConfigParams structure which can be used to initialize an FCSHorizontalStabilizerController instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/** 
			Constructor of the class.
			\param aircraft		:	The aircraft to which the FCS component is attached.
			\param name			:   Name of the FCS component instance.
			\param params		:	Configuration parameters of the FCS component.
		*/
		FCSHorizontalStabilizerController( RTD::VehicleBase* aircraft, const std::string& name , const ConfigParams* params );

		/** 
			Destructor of the class.
		*/
		virtual ~FCSHorizontalStabilizerController();


		/** Returns the type of the FCS component. For this class it returns "FCSHorizontalStabilizerController". */
		virtual std::string		getType() const;


		/** Updates the FCS component by advancing the simulation \e steptime seconds. */
		virtual void			update( smTime steptime );

		/** Resets the FCS component. */
		virtual void			reset();

		/** Returns the current incidence angle set by the controller. Return value is in degrees. */
		smReal	getIncidence() const	{	return o_incidenceAngle;	}
	protected:        
		ConfigParams	m_parameters;

		RTD::PilotInputData*	getOutputAsPilotInputData();

		RTD::OutputVariableReal	o_incidenceAngle;
	};

}