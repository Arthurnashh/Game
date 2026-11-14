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
#include "RTD/WorldParticipant.h"
#include "RTD/FGGroundContactBase.h"
#include "RTD/FGAirDrag100.h"

namespace RTD
{

class VehicleBase;

/**
	This class is a callback base class for terrain queries. Interaction with terrain is computed
	using ground contact models (classes derived from FGGroundContactBase). These models require information
	about the terrain to compute the forces and moments that act on the vehicle.

	A terrain query callback provides a ground contact model with the necessary interface to query the required
	information. Users have to derive a class from TerrainQueryCallback and implement the query() method within
	their own system. 
	
	Ground contacts call TerrainQueryCallback::query() every frame to ask for the terrain information they need.

	Deleting a vehicle does \b not result in deletion of the callback instance. This is, however, rarely necessary because
	TerrainQueryCallback instances are deleted automatically when the World to which they belong is deleted. But if you
	need to delete the callback yourself, remove() method of the callback has to be called. Note that, if an active vehicle class 
	is still using the callback, deleting the callback might result in a crash in the next update.

	@see TraitGroundContact::setTerrainQueryCallback
*/
class RTDYNAMICS_EXPORT TerrainQueryCallback : public WorldParticipant
{
public:

	/** Constructor of the class. */
	TerrainQueryCallback( RTD::World* world );

	/**
		This method is called by ground contacts periodically with different parameters to get information
		about the terrain. It should be implemented by the user. 

		A terrain query is a linear intersector between a start and end point. Users should construct a line
		between \e start and \e end and intersect it with the terrain to compute information on position, 
		normal and surface velocity at the point of intersection. If there are multiple intersections the nearest
		one should be returned.

	    \return	This method shall return true if a collision is detected.
	            Otherwise false must be returned.
		\param	start		Starting position of the intersector line in WCS.
		\param	end			End position of the intersector line in WCS.
		\param	hitXYZ		Position of the intersection in WCS.
		\param	hitNormal	Surface normal at the intersection in WCS.
		\param	hitVelocity	Velocity of the surface at the position of intersection. If the intersection
							occurs with terrain velocity is a zero vector. In case a moving object is intersected
							(like a ship) then users should compute the velocity vector of the intersection point
							in WCS and return in this parameter. Note that the velocity of the intersection point 
							is not the same as the velocity of the object itself (except the object has no rotational
							motion at all).
		\param frictionFactor This variable is used as a means of implementing different surface types. Ground 
							  contact models use this variable to scale up/down the final friction. In the simplest
							  case, a ground contact model multiplies its own friction coefficient with the one
							  provided here to obtain the final friction coefficient for the surface. Ground contact
							  models that use more complex models for friction may use this parameter in a different
							  way but in all cases the final effect is the same. 

	    \param isLiquid       This parameter must be set to true if the ground is liquid (e.g. sea).


	*/
	virtual bool query( const smVec3d& start, const smVec3d& end ,
						smVec3d& hitXYZ , smVec3d& hitNormal, smVec3d& hitVelocity,
						smReal& frictionFactor,
						bool& isLiquid);		
	protected:
		/** Destructor of the class. */
		virtual ~TerrainQueryCallback();

};

/**
*   The classes which are derived from this class gain terrain interaction
*   capabilities. It creates and manages ground contact model instances. Ground
*   contact models are classes which are derived from FGGroundContactBaseBase.
*
*	@see FGGroundContactBase
*/
class RTDYNAMICS_EXPORT TraitGroundContact
{
public:

	/** Constructor of the class. */
	TraitGroundContact();

	/** Destructor of the class. */
	virtual ~TraitGroundContact();
	

	/** Call this method to reset all FGGroundContactBases at once. */
	void	reset() ;

	/**
		Creates a ground contact instance of the given class type with user provided parameters.
		The ground contact will be attached to the vehicle.
	*/
	virtual FGGroundContactBase*	createGroundContact(	const std::string& className ,
															const std::string& name,
															const FGGroundContactBase::ConfigParams* parameters );


	/** Sets the terrain height (in meters) in current position of the vehicle.
	*   Simple ground contact models, such as FGGroundContact100, uses this
	*   information for terrain interaction computations. Users should compute
	*   the height of the terrain every frame and pass it to the vehicle using
	*   this method.
		
	*   In case a terrain query callback is provided ground contacts should use
	*   the provided callback for getting information on the terrain and
	*   setTerrainHeight() should \b not be called.
	*  
	*	@see TerrainQueryCallback|
	*/
	virtual void	setTerrainHeight( smReal terrainHeight  ) ;

	/**
		Returns true if any of the contacts are touching the ground.
	*/
	bool			isTouchingGround() const;

	/** Returns the terrain height (in meters) as known by this class. This
	*   method does
	*   \b not make any terrain intersections. It just returns the value set by
	*   setTerrainHeight().
	*
	*	@see setTerrainHeight
	*/
	smReal			getTerrainHeight() const {	return i_terrainHeight;	}

	/** 
	*	Returns true if the terrain below the vehicle is a liquid substance
	*   ( e.g. sea )
	*/
	smReal			isGroundLiquid() const {	return s_isGroundLiquid;	}

	/** Returns a list of all ground contacts regardless of their type. */
	virtual void			getGroundContacts( std::vector<FGGroundContactBase*>& contacts );

	/** Returns the number of ground contacts. */
	unsigned int			getNumGroundContacts() const;

	/**
		Returns the ground contact with the given index. If index is out of
		bounds NULL is returned.
	*/
	RTD::FGGroundContactBase*		getGroundContact(unsigned int);

	/**
		Returns the ground contact with the given index. If index is out of
		bounds NULL is returned.
	*/
	const RTD::FGGroundContactBase*	getGroundContact(unsigned int) const;

	/**
		Removes the ground contact with given index. If there isn't any ground contact at the given index
		nothing will be done.

		If \e deletePointer parameter is true then the ground contact will also be deleted using \b delete
		operator.

		Note that removing ground contact might change the indices of the remaining components.
	*/
	virtual void					removeGroundContact( unsigned int index , bool deletePointer );

	/**
		Returns a value in [0,1] range which indicates how much extended the landing gears are.
		0 Means gears are fully retracted.
		1 Means gears are fully extended.
	*/
	smReal								getGearStatus() const;

	/**
		Valid values of \e extensionAmount is in [0,1] range.
		0 Means gears are fully retracted.
		1 Means gears are fully extended.
	*/
	virtual void						setGearStatus( smReal extensionAmount );



	/**
		FGGroundContactBase is derived from ForceGenerator class hence it is updated automatically
		by the RigidBody when it is updated. This update method does not update FGGroundContactBase themselves.

		The update does following:
		- Ground contacts are checked to see whether they exceed maximum allowed loads (if so
		  areAllContactsIntact() will return false ).
		- Pedal and wheel brake pilot inputs are applied to gears.
		- Extension/retraction of the landing gears are simulated.
		- Air drag due to landing gears is computed.

		@see areAllContactsIntact
	*/
	virtual	void	update( smTime steptime );

	/**
		If none of the FGGroundContactBases exceeds its maximum load then areAllContactsIntact will
		return true. If there is at least one ground contact which exceeds its load limits then
		false will be returned.
	*/
	bool	areAllContactsIntact() const	{	return s_allContactsIntact;	}

	/** Sets the terrain query callback to be used by the ground contact models. If a terrain query callback
		is provided setTerrainHeight() should not be called anymore.

		@see TerrainQueryCallback
	*/
	virtual	void			setTerrainQueryCallback(  TerrainQueryCallback* tqc );

	/** Returns the terrain query callback instance used by the ground contact models. */
	TerrainQueryCallback*	getTerrainQueryCallback() const							{	return	m_terrainQueryCallback;	}


	// Experimental, please do not use.
	void					setPositionOnGround( const smVec2r& pos );

	

protected:
	std::vector<FGGroundContactBase*>	m_groundContacts;
	RTD::FGAirDrag100*					m_gearDrag;

	
	TerrainQueryCallback*			m_terrainQueryCallback;
	
	RTD::InputVariableReal			i_terrainHeight;
	RTD::InputVariableBool			s_isGroundLiquid;
	RTD::StateVariableBool			s_allContactsIntact;
	RTD::StateVariableReal			s_gearExtensionAmount;

	smReal							m_landingGearExtensionTime;
	smReal							m_maximumSteerableGearDeflection;
	smReal							m_steerableGearSpeedCorrectionFactor;
	smReal							m_landingGearReferenceArea;
	smReal							m_landingGearDragCoeff;

	virtual void					registerSimulationVariables();

	void cleanUp();
};

}