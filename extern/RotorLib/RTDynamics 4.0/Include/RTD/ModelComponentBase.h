/*
(c) Copyright 2007, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/Base.h"
#include "RTD/ParamDb.h"

namespace RTD
{
	class VehicleBase;

	/**
		This class serves as a base class for components of simulation models. It provides an interface for
		- Creation by class name
		- Creation of configuration parameters by class name
		- Loading configuration parameters from XML files

		A new component class can be created as follows:
		- Derive a class from ModelComponentBase.
		- Override getType() method to return the class name as string (E.g. ConstantRPMEngine class returns "ConstantRPMEngine" ).
		- Implement a static creator function that creates a new instance of the component.
		- Within the new component class derive a ConfigParams class which is derived from ModelComponentBase::ConfigParams.
		- Implement a static creator function that creates a new instance of the component's ConfigParams class.
		- Implement readParamStr() method of the new ConfigParams class. This method is needed to load parameters from XML files.
		- Register the new component using registerClass() method.

		@see ConstantRPMEngine
		@see ControlSystemComponentBase

	*/
	class RTDYNAMICS_EXPORT ModelComponentBase : public virtual SimulationVariableUser
	{
	public:

		/**
			Configuration parameters of the component. This class serves as a base class
			for ConfigParams classes of the derived components.
		*/
		class RTDYNAMICS_EXPORT ConfigParams
		{
		public:
			virtual ~ConfigParams();

			/**
				When a helicopter or airplane is configured from an XML configuration file sections related
				to a component is handed to the component itself. This is done by calling readParamStr() method
				of the ConfigParams. ConfigParams should interpret the string that contains the configuration
				information and set its member variables accordingly.

				\param parameterString	This string contains the configuration information in XML.
				\param error			This is an output parameter. If an error is detected then \e
										error should contain the error message and the method should return false.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/**
			Prototype of a component creator function.
		*/
		typedef 	ModelComponentBase*				  (*CreatorFuncType)( RTD::VehicleBase* parentVehicle , const std::string& name , const ConfigParams* params  );

		/**
			Prototype of a component parameters creator function.
		*/
		typedef 	ModelComponentBase::ConfigParams*  (*ParametersCreatorFuncType)();

		/**
			This method is used to register a new component to the system.
			\param className		Name of the new component class. This name should be the same as the one returned by the getType() method.
			\param creator			A creator function which can be called to create a new instance of the component.
			\param paramsCreator	A creator function which can be called to create a new instance of the component's parameters class.
		*/
		static	void				registerClass( const std::string& className , CreatorFuncType creator , ParametersCreatorFuncType paramsCreator );

		/**
			This method is cleans up the registry.
		*/
		static	void				deregisterClasses( );

		/**
			Returns a pointer to the creator function of the component class with name \e className.
		*/
		static	CreatorFuncType		getCreatorFunc( const std::string& className );

		/**
			Returns a pointer to the creator function of the ConfigParams class of a component
			with name \e className.
		*/
		static	ParametersCreatorFuncType	getParametersCreatorFunc( const std::string& className );

		/**
			Returns the name of the class. This should be the same as the one registered with registerClass() method.
		*/
		virtual std::string		getType() const	=	0;

		/**
			This method creates a new instance of a component with given class name.

			\param	className	Class name (type).
			\param	vehicle		The vehicle to which the new component will be attached.
			\param	name		Name of the new instance.
			\param	parameters	Configuration parameters of the new instance.

			@see createParameters
		*/
		static	ModelComponentBase*				createClass( const std::string& className , RTD::VehicleBase* vehicle , const std::string& name, const ConfigParams* parameters );

		/**
			This method creates a new instance of the ConfigParams class of given component type. After creating
			and configuring the ConfigParams you can call createClass() to create the new component instance.

			@see createClass
		*/
		static	ModelComponentBase::ConfigParams*	createParameters( const std::string& className );

		/**
			Constructor of the component.

			\param	vehicle		The vehicle to which the new component will be attached.
			\param	name		Name of the new instance.
			\param	parameters	Configuration parameters of the new instance.

		*/
		ModelComponentBase( RTD::VehicleBase* vehicle, const std::string& name, const ConfigParams* parameters  );

		/**
			Destructor of the class.
		*/
		virtual ~ModelComponentBase();

		/** Updates the component by advancing the simulation \e stepTime seconds. */
		virtual void			update( smTime stepTime )	=	0;

		/** Return the aircraft to which this component is attached. */
		RTD::VehicleBase*	getAttachedVehicle() const	{	return m_attachedVehicle;	}

	protected:
		struct ModelComponentRegistryEntity
		{
			std::string					className;
			CreatorFuncType				creatorFunc;
			ParametersCreatorFuncType	paramsCreatorFunc;

		};
		static	std::vector<ModelComponentRegistryEntity>	s_modelComponentRegistry;

		RTD::VehicleBase*	m_attachedVehicle;

	};


}