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
#include "RTD/GenericModel.h"
#include "Physics/smGraph2D.h"

namespace RTD
{

	class VehicleBase;

    /**
        This class implements a generic control system component using GenericModel class. Using this class,
        developers can implement complex control system functions within the XML configuration file without
        C++ programming.
        
        See Generic Control System section in the Programmer's Guide for more information.
    */
    class RTDYNAMICS_EXPORT GenericControlSystem :	public RTD::ControlSystemComponentBase
	{
	public:

		/**
			Configuration parameters of the controller.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::ControlSystemComponentBase::ConfigParams
		{
		public:
			ConfigParams();

            /** An XML based description of the generic model that is executed as control system. This text string
                is given to GenericModel or parsing. */
            std::string         m_description;
			
			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool		readParamStr( const std::string& parameterString , std::string& error );
		};

		/**
			Creates an instance of GenericControlSystem.
			\param vehicle		:	The vehicle to which the GenericControlSystem is attached.
			\param name			:   Name of the GenericControlSystem instance.
			\param parameters	:	Configuration parameters of the GenericControlSystem.
		*/
		static	RTD::ControlSystemComponentBase*					createClass( RTD::VehicleBase* vehicle, const std::string& name , const ConfigParams* parameters  );

		/**
			Creates a ConfigParams structure which can be used to initialize an GenericControlSystem instance.
		*/
		static	RTD::ControlSystemComponentBase::ConfigParams*	createParameters();

		/**
		*	Constructor of the class.
		*   \param parentVh		:	The vehicle to which the GenericControlSystem
		*          					is attached.
		*	\param name			:   Name of the GenericControlSystem instance.
		*	\param parameters	:	Configuration parameters of the GenericControlSystem.
		*/
		GenericControlSystem( RTD::VehicleBase* parentVh, const std::string& name , const ConfigParams* parameters );

		/**
			Destructor of the class.
		*/
		virtual ~GenericControlSystem();


		/** Returns "GenericControlSystem". */
		virtual std::string		getType() const;


        /** */
		virtual void			update( smTime steptime );

        /** */
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

		ConfigParams*		m_configParams;
        GenericModel*       m_modelDescription;

	};

}