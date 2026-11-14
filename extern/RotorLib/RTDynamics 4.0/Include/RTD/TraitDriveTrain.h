/*
(c) Copyright 2007, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once


#include "RTD/DriveTrainModelBase.h"

namespace RTD
{

class VehicleBase;

/**
	This trait class provides the derived classes with DriveTrainModelBase instance creation and management
	capabilities.

	@see DriveTrainModelBase
*/
class RTDYNAMICS_EXPORT TraitDriveTrain
{
public:

	/** Constructor of the class. */
	TraitDriveTrain();

	/** Destructor of the class. Deletes all drive trains and removes their force generators from
		the vehicle's force generator list. */
	virtual ~TraitDriveTrain();

	/**
		Creates a drive train instance of the given class type with user provided parameters.
		The drive train will be attached to the aircraft and if it has a force generator
		it will be added to the force generators list.
	*/
	DriveTrainModelBase*	createDriveTrain(	const std::string& className ,
												const std::string& name,
												const DriveTrainModelBase::ConfigParams* parameters );

	/**
		Gets the number of drive trains.
	*/
	unsigned int				getNumDriveTrains() const;

	/**
		Gets the drive train with given index. If there isn't any drive train at the given index NULL is returned.

		Note that removing drive trains might change the indices of the remaining drive trains.
	*/
	const RTD::DriveTrainModelBase*	getDriveTrain( unsigned int index )	const;

	/**
		Gets the drive train with given index. If there isn't any drive train at the given index NULL is returned.

		Note that removing drive trains might change the indices of the remaining drive trains.
	*/
	RTD::DriveTrainModelBase*			getDriveTrain(unsigned int index );

	/**
		Removes the drive train with given index. If the index exceeds the number of drive trains nothing will
		be done.

		If \e deletePointer parameter is true then the drive train will also be deleted using \b delete
		operator.

		Note that removing drive trains might change the indices of the remaining drive trains.
	*/
	virtual void					removeDriveTrain( unsigned int index , bool deletePointer );

	/**
		Calls reset on all drive trains.
	*/
	virtual void	reset();


	/**
		Calls update on all drive trains.
	*/
	virtual void	update( smTime steptime );

protected:
	std::vector<RTD::DriveTrainModelBase*>	m_driveTrains;

	void cleanUp();

};

}