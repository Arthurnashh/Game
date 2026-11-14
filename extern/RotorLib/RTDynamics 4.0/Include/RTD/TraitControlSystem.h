/*
(c) Copyright 2007, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once


#include "RTD/ControlSystemComponentBase.h"

namespace RTD
{

class VehicleBase;

/**
	This trait class provides the derived classes with ControlSystemComponentBase instance creation and management
	capabilities.

	@see ControlSystemComponentBase
*/
class RTDYNAMICS_EXPORT TraitControlSystem
{
public:

	/** Constructor of the class. */
	TraitControlSystem();

	/** Destructor of the class. Deletes all control system components. */
	virtual ~TraitControlSystem();


	/**
		Creates a flight control system component instance of the given class type with user
		provided parameters.
	*/
	ControlSystemComponentBase*	createCSComponent(	const std::string& className ,
											const std::string& name,
											const ControlSystemComponentBase::ConfigParams* parameters	);

	/**
		Returns the number of control system components.
	*/
	unsigned int				getNumCSComponents() const;

	/**
		Returns the control system component with given index. If there isn't any control system component at the given index NULL is returned.
	*/
	const RTD::ControlSystemComponentBase*	getCSComponent( unsigned int i )	const;

	/**
		Returns the control system component with given index. If there isn't any control system component at the given index NULL is returned.
	*/
	RTD::ControlSystemComponentBase*		getCSComponent(unsigned int i );

	/**
        Returns the first control system component of type \e className. The method returns NULL if no components of the given type is found.
    */
	const RTD::ControlSystemComponentBase*		getCSComponentByType( const std::string& className ) const;

	/**
        Returns the first control system component of type \e className. The method returns NULL if no components of the given type is found.
    */
	RTD::ControlSystemComponentBase*		getCSComponentByType( const std::string& className ) ;

	/**
		Removes the control system component with given index. If there isn't any control system component at the given index
		nothing will be done.

		If \e deletePointer parameter is true then the component will also be deleted using \b delete
		operator.

		Note that removing components might change the indices of the remaining components.
	*/
	virtual void					removeCSComponent( unsigned int index , bool deletePointer );

	/**
		Calls reset on all control system components.
	*/
	virtual void	reset();

	/**
        Returns the output of the last control system stage, i.e. last control system component in the cascade.
    */
	virtual const InputData*	getFinalInputData() const;

	/**
		Calls update on all control system components.
	*/
	virtual void	update( smTime steptime );


protected:
	void	cleanUp();

private:
	std::vector<RTD::ControlSystemComponentBase*>	m_csComponents;

	

};

}